#include "SpeechToText/AudioCapture.h"
#include "SpeechToText/ElevenLabsSttClient.h"
#include "SpeechToText/Base64.h"

#include <cstdlib>
#include <iostream>
#include "ChatGPT/ChatGPT.h"

//For file storing
#include <fstream>
#include <iostream>
#include <string>

//For having Pilot be active or not active
#include <atomic>

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

    const char* key = std::getenv("ELEVENLABS_API_KEY");
    if (!key) {
        std::cerr << "Missing ELEVENLABS_API_KEY\n";
        return 1;
    }
    
    ElevenLabsSttClient stt;
    std::set<std::string> wakePhrase = {
        "Hey, Pilot.", "Hey Pilot.", "Hey Pilot", 
        "Hey, Pilot.", "hey Pilot", "hey pilot", 
        "hey, Pilot", "hey, pilot", "hey Pilot,", 
        "Hey Pilot,", "Hey pilot,", "Hey, Pilot,", 
        "Hey, pilot,", "pilot", "Pilot", "Pilot,",
        "pilot",
    };

    std::set<std::string> sleepPhrase = {
        "Thank You", "Thank You.", "Thank, You", "Thank, You.",
        "thank you", "thank you.", "thank, you", "thank you.",
        "Thank you.",
    };

    //File creation to store information and logging.
    std::ofstream outFile("ConversationFile.txt", std::ios::app);
    outFile.flush();
    std::cout << "Log file open? " << outFile.is_open() << std::endl;

    stt.setCallback([&wakePhrase, &pilotActive, &outFile, &sleepPhrase](const std::string &type, const std::string &text) {
        if (type == "committed_transcript" || type == "committed_transcript_with_timestamps") {

            //If Pilot is NOT active: only look for wake words
            if (!pilotActive.load())
            {
                for (const auto& phrase : wakePhrase)
                {
                    if (text.find(phrase) != std::string::npos)
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
            for (const auto& sleep : sleepPhrase)
            {
                if (text.find(sleep) != std::string::npos)
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

                std::cout << "User: " << text << "\n";
                std::cout << "Pilot: " << reply << "\n";

                if (outFile.is_open())
                {
                    outFile << "User: " << text << std::endl;
                    outFile << "Pilot: " << reply << std::endl;
                }
                outFile.flush();
            }
            catch (const std::exception& e)
            {
                std::cerr << "Pilot AI Error: " << e.what() << "\n";
            }
        } 
    });

    stt.connectVadPcm16000(key);

    AudioCapture mic(16000, 320);
    mic.start();

    std::vector<int16_t> samples;
    while (true) {
        mic.read(samples);
        auto b64 = base64_encode(
            reinterpret_cast<uint8_t*>(samples.data()),
            samples.size() * sizeof(int16_t)
        );
        stt.sendPcmChunkBase64(b64, 16000, false);
    }

    //Closing File
    outFile.close();

    #ifdef _WIN32
    WSACleanup();
    #endif
}
