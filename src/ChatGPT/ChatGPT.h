#pragma once

#include <string>

namespace ChatGPT {

    /**
     * Sends a prompt to OpenAI and returns the model's response text.
     * Throws std::runtime_error on failure.
     */
    std::string Ask(const std::string& prompt);

}
