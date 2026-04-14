#pragma once
#include <string>

namespace Commands {

    // Checks the transcript against all known commands.
    // Returns true if a command matched and was executed (caller should skip ChatGPT).
    // Returns false if no command matched (caller should send to ChatGPT as normal).
    bool TryHandle(const std::string& text);

    // Executes a tool call dispatched from ChatGPT function calling.
    // name     — the function name ChatGPT chose (e.g. "create_folder")
    // argsJson — JSON string of arguments (e.g. {"name":"Projects","location":"desktop"})
    // Returns a plain-English spoken confirmation ready for TTS.
    std::string ExecuteTool(const std::string& name, const std::string& argsJson);

}
