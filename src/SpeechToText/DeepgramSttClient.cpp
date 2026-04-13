#include "DeepgramSttClient.h"
#include <nlohmann/json.hpp>
#include <iostream>

using json = nlohmann::json;

DeepgramSttClient::DeepgramSttClient() {}
DeepgramSttClient::~DeepgramSttClient() { close(); }

void DeepgramSttClient::setCallback(Callback cb) { cb_ = std::move(cb); }

bool DeepgramSttClient::connectPcm16000(const std::string& apiKey,
                                        const std::string& model,
                                        bool interim_results,
                                        bool smart_format,
                                        int utterance_end_ms)
{
    // Deepgram live streaming endpoint
    // Use encoding + sample_rate for raw PCM streaming. :contentReference[oaicite:1]{index=1}
    std::string url = "wss://api.deepgram.com/v1/listen"
                      "?encoding=linear16"
                      "&sample_rate=16000"
                      "&channels=1";

    url += "&model=" + model; // e.g. nova-3 :contentReference[oaicite:2]{index=2}
    url += std::string("&interim_results=") + (interim_results ? "true" : "false");
    url += std::string("&smart_format=") + (smart_format ? "true" : "false");
    url += "&utterance_end_ms=" + std::to_string(utterance_end_ms); // end-of-speech helper :contentReference[oaicite:3]{index=3}

    ws_.setUrl(url);

    ws_.setExtraHeaders({
        {"Authorization", "Token " + apiKey}
    });

    ws_.setOnMessageCallback([this](const ix::WebSocketMessagePtr& msg) {
        handleMessage(msg);
    });

    auto res = ws_.connect(5);
    if (!res.success) {
        std::cerr << "[DeepgramSTT] connect failed: " << res.errorStr << "\n";
        return false;
    }

    isOpen_.store(true);
    ws_.start();
    return true;
}

void DeepgramSttClient::sendPcmSamples(const int16_t* samples, size_t sampleCount)
{
    if (!isOpen_.load()) return;
    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(samples);
    size_t byteCount = sampleCount * sizeof(int16_t);

    ws_.sendBinary(std::string(reinterpret_cast<const char*>(bytes), byteCount));
}

void DeepgramSttClient::finalize()
{
    if (!isOpen_.load()) return;

    // Deepgram supports sending a Finalize message to flush the stream. :contentReference[oaicite:4]{index=4}
    json j;
    j["type"] = "Finalize";
    ws_.sendText(j.dump());
}

void DeepgramSttClient::close()
{
    if (!isOpen_.exchange(false)) return;
    try {
        ws_.stop();
    } catch (...) {}
}

void DeepgramSttClient::handleMessage(const ix::WebSocketMessagePtr& msg)
{
    if (msg->type == ix::WebSocketMessageType::Message) {
        // Deepgram sends JSON messages with results; parse transcript if present.
        try {
            auto j = json::parse(msg->str);

            // Common Deepgram live messages include "Results" and flags like "is_final"/"speech_final".
            // We'll emit:
            //  - "committed_transcript" when final
            //  - "interim_transcript" otherwise
            std::string transcript;

            if (j.contains("channel") && j["channel"].contains("alternatives") &&
                j["channel"]["alternatives"].is_array() && !j["channel"]["alternatives"].empty() &&
                j["channel"]["alternatives"][0].contains("transcript"))
            {
                transcript = j["channel"]["alternatives"][0]["transcript"].get<std::string>();
            }

            if (!transcript.empty() && cb_) {
                bool isFinal = false;
                if (j.contains("is_final"))      isFinal = j["is_final"].get<bool>();
                if (j.contains("speech_final"))  isFinal = j["speech_final"].get<bool>();

                cb_(isFinal ? "committed_transcript" : "interim_transcript", transcript);
            }

        } catch (const std::exception& e) {
            std::cerr << "[DeepgramSTT] JSON parse error: " << e.what() << "\n";
        }
    }
    else if (msg->type == ix::WebSocketMessageType::Open) {
        // connected
    }
    else if (msg->type == ix::WebSocketMessageType::Error) {
        std::cerr << "[DeepgramSTT] websocket error: " << msg->errorInfo.reason << "\n";
    }
    else if (msg->type == ix::WebSocketMessageType::Close) {
        std::cout << "[DeepgramSTT] Connection closed. Code: " << msg->closeInfo.code << " Reason: " << msg->closeInfo.reason << "\n";
        isOpen_.store(false);
    }
}
