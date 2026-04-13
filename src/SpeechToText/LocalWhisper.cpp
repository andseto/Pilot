#include "LocalWhisper.h"
#include <whisper.h>
#include <iostream>

LocalWhisper::LocalWhisper() = default;

LocalWhisper::~LocalWhisper() {
    if (ctx_) whisper_free(ctx_);
}

bool LocalWhisper::load(const std::string& modelPath) {
    ctx_ = whisper_init_from_file(modelPath.c_str());
    if (!ctx_) {
        std::cerr << "[LocalWhisper] Failed to load model: " << modelPath << "\n";
        return false;
    }
    std::cout << "[LocalWhisper] Model loaded: " << modelPath << "\n";
    return true;
}

std::string LocalWhisper::transcribe(const std::vector<float>& samples) {
    if (!ctx_ || samples.empty()) return "";

    whisper_full_params params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
    params.print_progress   = false;
    params.print_special    = false;
    params.print_realtime   = false;
    params.print_timestamps = false;
    params.language         = "en";
    params.n_threads        = 4;
    params.single_segment   = true;
    params.no_context       = true;  // don't carry context between calls

    if (whisper_full(ctx_, params, samples.data(), (int)samples.size()) != 0) {
        std::cerr << "[LocalWhisper] Inference failed\n";
        return "";
    }

    std::string result;
    const int n = whisper_full_n_segments(ctx_);
    for (int i = 0; i < n; i++) {
        result += whisper_full_get_segment_text(ctx_, i);
    }
    return result;
}
