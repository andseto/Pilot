#pragma once
#include <vector>
#include <cstdint>

class AudioCapture {
public:
    AudioCapture(int sampleRate, int framesPerBuffer);
    ~AudioCapture();

    bool start();
    void stop();
    bool read(std::vector<int16_t>& out);

private:
    int sampleRate_;
    int framesPerBuffer_;
    void* stream_; // PaStream*
};
