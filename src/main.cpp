#include "SpeechToText/AudioCapture.h"
#include "SpeechToText/ElevenLabsSttClient.h"
#include "SpeechToText/Base64.h"

#include <cstdlib>
#include <iostream>

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

    const char* key = std::getenv("ELEVENLABS_API_KEY");
    if (!key) {
        std::cerr << "Missing ELEVENLABS_API_KEY\n";
        return 1;
    }

    ElevenLabsSttClient stt;
    std::set<std::string> wakePhrase = {
        "Hey, Cypher.", "Hey Cypher.", "Hey Cypher", 
        "Hey, Cypher.", "hey Cypher", "hey cypher", 
        "hey, Cypher", "hey, cypher", "hey Cypher,", 
        "Hey Cypher,", "Hey cypher,", "Hey, Cypher,", 
        "Hey, cypher,", "cypher", "Cypher", "Cypher,",
        "cypher"};

    stt.setCallback([&wakePhrase](const std::string& type, const std::string& text) {
        if (type == "committed_transcript" || type == "committed_transcript_with_timestamps") {
            for (const auto& phrase : wakePhrase) {
                if (text.find(phrase) != std::string::npos) {
                    std::cout << "Wake phrase detected\n";
                    break;
                }
            }

            //std::cout << text << "\n";
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

    #ifdef _WIN32
    WSACleanup();
    #endif
}
