#pragma once

#include <string>
#include <vector>
#include "../Memory/ConversationMemory.h"

namespace ChatGPT {

    // Sends a prompt to OpenAI, including prior conversation history.
    // Throws std::runtime_error on failure.
    std::string Ask(const std::string& prompt,
                    const std::vector<Message>& history = {});

}
