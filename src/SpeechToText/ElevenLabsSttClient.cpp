#include "ElevenLabsSttClient.h"
#include <ixwebsocket/IXWebSocket.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

ElevenLabsSttClient::ElevenLabsSttClient() : connected_(false), ws_(nullptr) {}
ElevenLabsSttClient::~ElevenLabsSttClient() { close(); }

void ElevenLabsSttClient::setCallback(TranscriptCallback cb) { cb_ = std::move(cb); }

bool ElevenLabsSttClient::connectVadPcm16000(const std::string& apiKey)
{
    auto* ws = new ix::WebSocket();

    std::string url =
        "wss://api.elevenlabs.io/v1/speech-to-text/realtime"
        "?audio_format=pcm_16000"
        "&commit_strategy=vad";

    ws->setUrl(url);
    ws->setExtraHeaders({ {"xi-api-key", apiKey} });

    ws->setOnMessageCallback([this](const ix::WebSocketMessagePtr& msg) {
        if (msg->type == ix::WebSocketMessageType::Open) connected_ = true;
        if (msg->type == ix::WebSocketMessageType::Close) connected_ = false;

        if (msg->type == ix::WebSocketMessageType::Message) {
            try {
                auto j = json::parse(msg->str);
                const std::string type = j.value("message_type", "");
                const std::string text = j.value("text", "");
                if (cb_ && !text.empty()) cb_(type, text);
            } catch (...) {}
        }
    });

    ws->start();
    ws_ = ws;
    return true;
}

void ElevenLabsSttClient::sendPcmChunkBase64(const std::string& b64Audio, int sampleRate, bool commit)
{
    if (!ws_ || !connected_) return;
    auto* ws = reinterpret_cast<ix::WebSocket*>(ws_);

    json out = {
        {"message_type", "input_audio_chunk"},
        {"audio_base_64", b64Audio},
        {"commit", commit},
        {"sample_rate", sampleRate}
    };

    ws->send(out.dump());
}

void ElevenLabsSttClient::close()
{
    if (!ws_) return;
    auto* ws = reinterpret_cast<ix::WebSocket*>(ws_);
    ws->stop();
    delete ws;
    ws_ = nullptr;
}
