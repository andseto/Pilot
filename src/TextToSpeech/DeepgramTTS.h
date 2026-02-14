#pragma once
#include <string>

// Returns true if audio successfully written to outFile (e.g., "reply.mp3")
bool Deepgram_TTS_ToFile(
    const std::string& apiKey,
    const std::string& model,
    const std::string& text,
    const std::string& outFile,
    const std::string& encoding = "mp3",
    const std::string& bitRate  = ""  // optional e.g. "32000"
);
