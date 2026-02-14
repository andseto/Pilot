#pragma once
#include <functional>
#include <string>
#include <atomic>
#include <ixwebsocket/IXWebSocket.h>

class DeepgramSttClient {
public:
    using Callback = std::function<void(const std::string& type, const std::string& text)>;

    DeepgramSttClient();
    ~DeepgramSttClient();

    void setCallback(Callback cb);

    // Connect Deepgram live STT for 16kHz mono linear16 PCM
    bool connectPcm16000(const std::string& apiKey,
                         const std::string& model = "nova-3",
                         bool interim_results = true,
                         bool smart_format = true,
                         int utterance_end_ms = 1200);

    // Send raw PCM samples (16-bit signed little-endian)
    void sendPcmSamples(const int16_t* samples, size_t sampleCount);

    // Optional: flush any buffered audio on server
    void finalize();

    void close();

private:
    void handleMessage(const ix::WebSocketMessagePtr& msg);

    ix::WebSocket ws_;
    Callback cb_;
    std::atomic<bool> isOpen_{false};
};
