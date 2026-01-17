#pragma once
#include <functional>
#include <string>
#include <atomic>

class ElevenLabsSttClient {
public:
    using TranscriptCallback = std::function<void(const std::string& type, const std::string& text)>;

    ElevenLabsSttClient();
    ~ElevenLabsSttClient();

    bool connectVadPcm16000(const std::string& apiKey);
    void setCallback(TranscriptCallback cb);

    void sendPcmChunkBase64(const std::string& b64Audio, int sampleRate, bool commit);
    void close();

private:
    TranscriptCallback cb_;
    std::atomic<bool> connected_;
    void* ws_; // ix::WebSocket*
};
