#include "SpeechToText/AudioCapture.h"
#include "SpeechToText/DeepgramSttClient.h"

#include <cstdlib>
#include <iostream>
#include "ChatGPT/ChatGPT.h"

//For file storing
#include <fstream>
#include <iostream>
#include <string>

//For having Pilot be active or not active
#include <atomic>

//For Text to Speech
#include "TextToSpeech/DeepgramTTS.h"
#include <cstdlib> 

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib, "Ws2_32.lib")
#endif
#include <set>


int main()
{
    #ifdef _WIN32
        WSADATA wsaData;
        int wsaErr = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (wsaErr != 0) {
            std::cerr << "WSAStartup failed: " << wsaErr << "\n";
            return 1;
        }
    #endif

    //Setting Pilot active listen or not
    std::atomic<bool> pilotActive{false};

    const char* key = std::getenv("DEEPGRAM_API_KEY");
    if (!key) {
        std::cerr << "Missing DEEPGRAM_API_KEY\n";
        return 1;
    }
    
    const std::string ttsModel = "aura-2-odysseus-en"; // your voice
    
    DeepgramSttClient stt;

    std::set<std::string> wakePhrase = {
        "hey pilot", "hey, pilot", "pilot",
    };

    std::set<std::string> sleepPhrase = {
        "thank you", "thank, you",
    };

    // Helper: convert string to lowercase for case-insensitive matching
    auto toLower = [](const std::string& s) {
        std::string result = s;
        for (char& c : result) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return result;
    };

    //File creation to store information and logging.
    std::ofstream outFile("ConversationFile.txt", std::ios::app);
    outFile.flush();


    stt.setCallback([&wakePhrase, &pilotActive, &outFile, &sleepPhrase, &ttsModel, &key, &toLower](const std::string &type, const std::string &text) {
        if (type == "committed_transcript" || type == "committed_transcript_with_timestamps") {

            //If Pilot is NOT active: only look for wake words
            if (!pilotActive.load())
            {
                const std::string lowerText = toLower(text);
                for (const auto& phrase : wakePhrase)
                {
                    if (lowerText.find(phrase) != std::string::npos)
                    {
                        std::cout << "Wake word detected : " << phrase << std::endl;
                        pilotActive.store(true);

                        std::cout << "Pilot is actively listening!" << std::endl;

                        if (outFile.is_open())
                        {
                            outFile << "Wake word detected : " << phrase << std::endl;
                            outFile << "Pilot: Online, sir. I'm listening." << std::endl;
                        }
                        outFile.flush();

                        return;
                    }
                }

                return;
            }

            //If Pilot IS active: check sleep words first
            const std::string lowerText = toLower(text);
            for (const auto& sleep : sleepPhrase)
            {
                if (lowerText.find(sleep) != std::string::npos)
                {
                    pilotActive.store(false);

                    std::cout << "Pilot: No problem, sir. Standing by." << std::endl;

                    if (outFile.is_open())
                    {
                        outFile << "User: " << text << std::endl;
                        outFile << "Pilot: No problem, sir. Standing by." << std::endl;
                    }
                    outFile.flush();

                    return;
                }
            }

            // 3) Pilot active + not sleeping: call ChatGPT once for this transcript
            try
            {
                std::string reply = ChatGPT::Ask(text);

                const char* dgKey = std::getenv("DEEPGRAM_API_KEY");

                if (dgKey != nullptr)
                {
                    const std::string audioFile = "reply.mp3";
                    const std::string ttsModel  = "aura-2-thalia-en";  // Deepgram voice model

                    if (Deepgram_TTS_ToFile(dgKey, ttsModel, reply, audioFile, "mp3"))
                    {
                        // Play audio using default Windows player
                        system(("cmd /c start \"\" \"" + audioFile + "\"").c_str());
                    }
                    else
                    {
                        std::cerr << "[Deepgram] TTS request failed.\n";
                    }
                }
                else
                {
                    std::cerr << "[Deepgram] Missing DEEPGRAM_API_KEY environment variable.\n";
                }

                std::cout << "User: " << text << "\n";
                std::cout << "Pilot: " << reply << "\n";

                if (outFile.is_open())
                {
                    outFile << "User: " << text << std::endl;
                    outFile << "Pilot: " << reply << std::endl;
                    outFile.flush();
                }
            }
            catch (const std::exception& e)
            {
                std::cerr << "Pilot AI Error: " << e.what() << "\n";
            }
        } 
    });

    if (!stt.connectPcm16000(key, "nova-3", true, true, 1200)) {
        std::cerr << "Deepgram STT connect failed\n";
        return 1;
    }

    AudioCapture mic(16000, 320);
    if (!mic.start(3)) { // device 3 = Logitech PRO X Gaming Headset
        std::cerr << "Failed to open microphone. Check mic permissions and default recording device.\n";
        return 1;
    }

    std::vector<int16_t> samples;
    while (true) {
        mic.read(samples);
        stt.sendPcmSamples(samples.data(), samples.size());
    }

    //Closing File
    outFile.close();

    #ifdef _WIN32
    WSACleanup();
    #endif
}
