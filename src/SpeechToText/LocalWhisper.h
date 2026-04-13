#pragma once
#include <string>
#include <vector>
#include <cstdint>

struct whisper_context;

class LocalWhisper {
public:
    LocalWhisper();
    ~LocalWhisper();

    // Load a ggml model file (e.g. models/ggml-base.en.bin)
    bool load(const std::string& modelPath);

    // Transcribe a buffer of 16 kHz mono float32 samples.
    // Returns the recognised text, or empty string on failure.
    std::string transcribe(const std::vector<float>& samples);

private:
    whisper_context* ctx_ = nullptr;
};
