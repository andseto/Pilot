#include "SpeechToText/AudioCapture.h"
#include "SpeechToText/DeepgramSttClient.h"
#include "SpeechToText/LocalWhisper.h"

#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <chrono>
#include <future>
#include <thread>
#include <iostream>
#include "ChatGPT/ChatGPT.h"

#include <fstream>
#include <string>
#include <atomic>
#include <cctype>
#include <set>

#include "TextToSpeech/DeepgramTTS.h"
#include "Audio/AudioPlayer.h"

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib, "Ws2_32.lib")
#endif


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

    const char* key = std::getenv("DEEPGRAM_API_KEY");
    if (!key) {
        std::cerr << "Missing DEEPGRAM_API_KEY\n";
        return 1;
    }

    const std::string ttsModel = "aura-2-thalia-en";

    // ── Local Whisper for wake word detection (no API cost) ────────────────
    LocalWhisper localWhisper;
    if (!localWhisper.load("models/ggml-base.en.bin")) {
        std::cerr << "Place ggml-base.en.bin in a 'models/' folder next to the exe.\n";
        return 1;
    }

    std::set<std::string> wakePhrase  = { "hey pilot", "hey, pilot", "pilot" };
    std::set<std::string> sleepPhrase = { "thank you", "thank, you" };

    auto toLower = [](const std::string& s) {
        std::string r = s;
        for (char& c : r) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return r;
    };

    std::ofstream outFile("ConversationFile.txt", std::ios::app);

    // Microphone stays open the whole time (always capturing audio)
    AudioCapture mic(16000, 320);
    if (!mic.start(3)) {
        std::cerr << "Failed to open microphone.\n";
        return 1;
    }

    std::vector<int16_t> samples;
    std::vector<float>   whisperBuf;
    const int WHISPER_WINDOW = 16000 * 2; // 2 seconds of audio at 16 kHz

    while (true)
    {
        // ══════════════════════════════════════════════════════════════════
        // PHASE 1 — Local wake word detection (FREE, no API calls)
        // ══════════════════════════════════════════════════════════════════
        std::cout << "[Pilot] Waiting for wake word...\n";
        whisperBuf.clear();
        bool wakeDetected = false;

        while (!wakeDetected) {
            mic.read(samples);

            // Convert int16 → float32 in [-1, 1] range for Whisper
            for (auto s : samples)
                whisperBuf.push_back(s / 32768.0f);

            // Run inference every 2 seconds of buffered audio
            if ((int)whisperBuf.size() >= WHISPER_WINDOW) {
                std::string text = localWhisper.transcribe(whisperBuf);
                whisperBuf.clear();

                if (!text.empty()) {
                    const std::string lower = toLower(text);
                    for (const auto& phrase : wakePhrase) {
                        if (lower.find(phrase) != std::string::npos) {
                            std::cout << "[Pilot] Wake word detected: \"" << phrase << "\"\n";
                            outFile << "Wake word detected: " << phrase << "\n";
                            outFile << "Pilot: Online, sir. I'm listening.\n";
                            outFile.flush();
                            wakeDetected = true;
                            break;
                        }
                    }
                }
            }
        }

        // ══════════════════════════════════════════════════════════════════
        // PHASE 2 — Remote Deepgram session (PAID, only while active)
        // ══════════════════════════════════════════════════════════════════

        // Greet based on time of day (Phoenix MST = UTC-7, no DST)
        {
            auto now  = std::chrono::system_clock::now();
            std::time_t t = std::chrono::system_clock::to_time_t(now);
            t -= 7 * 3600; // shift to MST
            struct tm mst{};
            gmtime_s(&mst, &t);
            bool isMorning = (mst.tm_hour < 10) || (mst.tm_hour == 10 && mst.tm_min == 0);
            PlayAudioFile(isMorning ? "GoodMorning_Sir.mp3" : "GoodEvening_Sir.mp3");
        }

        std::atomic<bool> pilotActive{true};

        DeepgramSttClient stt;
        stt.setCallback([&](const std::string& type, const std::string& text) {
            if (type != "committed_transcript" && type != "committed_transcript_with_timestamps")
                return;

            const std::string lower = toLower(text);

            // Sleep phrase → end session
            for (const auto& phrase : sleepPhrase) {
                if (lower.find(phrase) != std::string::npos) {
                    pilotActive.store(false);
                    std::remove("reply.mp3");
                    std::cout << "Pilot: No problem, sir. Standing by.\n";
                    PlayAudioFile("NP_CallOnMeAnyTime.mp3");
                    if (outFile.is_open()) {
                        outFile << "User: "  << text << "\n";
                        outFile << "Pilot: No problem, sir. Standing by.\n";
                        outFile.flush();
                    }
                    return;
                }
            }

            // Active command → ChatGPT + TTS
            try {
                // Kick off ChatGPT immediately in the background
                auto futureReply = std::async(std::launch::async, [&text]() {
                    return ChatGPT::Ask(text);
                });

                // 1 second pause before feedback clip
                std::this_thread::sleep_for(std::chrono::seconds(1));

                // Play understood feedback — blocks until the clip finishes
                PlayAudioFileSync("UnderStood_FeedBack.mp3");

                // 1 second pause after feedback before delivering the reply
                std::this_thread::sleep_for(std::chrono::seconds(1));

                // Get the reply (waits here only if the API isn't done yet)
                std::string reply = futureReply.get();

                const char* dgKey = std::getenv("DEEPGRAM_API_KEY");
                if (dgKey) {
                    const std::string audioFile = "reply.mp3";
                    if (Deepgram_TTS_ToFile(dgKey, ttsModel, reply, audioFile, "mp3"))
                        PlayAudioFile(audioFile);
                    else
                        std::cerr << "[Deepgram] TTS request failed.\n";
                }

                std::cout << "User: "  << text  << "\n";
                std::cout << "Pilot: " << reply << "\n";

                if (outFile.is_open()) {
                    outFile << "User: "  << text  << "\n";
                    outFile << "Pilot: " << reply << "\n";
                    outFile.flush();
                }
            } catch (const std::exception& e) {
                std::cerr << "Pilot AI Error: " << e.what() << "\n";
            }
        });

        if (!stt.connectPcm16000(key, "nova-3", true, true, 1200)) {
            std::cerr << "[Pilot] Deepgram connect failed — returning to local mode.\n";
            continue; // back to Phase 1
        }

        // Stream audio to Deepgram until sleep phrase is spoken
        while (pilotActive.load()) {
            mic.read(samples);
            stt.sendPcmSamples(samples.data(), samples.size());
        }

        stt.finalize();
        stt.close();
        std::cout << "[Pilot] Session ended. Returning to local wake word detection.\n";
        // Loop back to Phase 1
    }

    outFile.close();

    #ifdef _WIN32
    WSACleanup();
    #endif
}
