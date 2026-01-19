#include "ChatGPT.h"

#include <cstdlib>
#include <stdexcept>

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

namespace ChatGPT {

    static std::string get_env_or_throw(const char* name) {
        const char* v = std::getenv(name);
        if (!v || std::string(v).empty()) {
            throw std::runtime_error(std::string("Missing environment variable: ") + name);
        }
        return std::string(v);
    }

    static std::string extract_output_text(const nlohmann::json& j) {
        std::string out;

        if (!j.contains("output") || !j["output"].is_array())
            return out;

        for (const auto& item : j["output"]) {
            if (!item.contains("content") || !item["content"].is_array())
                continue;

            for (const auto& c : item["content"]) {
                if (c.contains("type") &&
                    c["type"] == "output_text" &&
                    c.contains("text")) {
                    out += c["text"].get<std::string>();
                }
            }
        }

        return out;
    }

    std::string Ask(const std::string& prompt) {
        const std::string api_key = get_env_or_throw("OPENAI_API_KEY");

        //Pre set prompt to give personality to Pilot
        const std::string system_prompt =
        "You are Pilot, a respectful, highly capable voice assistant. "
        "Speak in a professional, Jarvis-like tone: polite, confident, and concise. "
        "When the user asks for steps, give numbered steps. "
        "If you need assumptions, state them briefly and proceed. "
        "Do not mention you are an AI unless asked. "
        "Address the user as 'sir' unless they ask otherwise.";

        nlohmann::json body = {
            {"model", "gpt-4.1-mini"},
            {"input", nlohmann::json::array({
                {
                    {"role", "system"},
                    {"content", nlohmann::json::array({
                        {{"type", "input_text"}, {"text", system_prompt}}
                    })}
                },
                {
                    {"role", "user"},
                    {"content", nlohmann::json::array({
                        {{"type", "input_text"}, {"text", prompt}}
                    })}
                }
            })}
        };

        cpr::Response r = cpr::Post(
            cpr::Url{"https://api.openai.com/v1/responses"},
            cpr::Header{
                {"Authorization", "Bearer " + api_key},
                {"Content-Type", "application/json"}
            },
            cpr::Body{body.dump()}
        );

        if (r.error) {
            throw std::runtime_error("HTTP error: " + r.error.message);
        }

        if (r.status_code < 200 || r.status_code >= 300) {
            throw std::runtime_error(
                "OpenAI API error (" + std::to_string(r.status_code) + "):\n" + r.text
            );
        }

        auto json = nlohmann::json::parse(r.text);
        return extract_output_text(json);
    }

}
