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

    static std::string sanitizeForTTS(const std::string& input) {
        std::string s = input;

        // Replace common UTF-8 misencoded Windows-1252 sequences
        auto replace_all = [&](const std::string& from, const std::string& to) {
            size_t pos = 0;
            while ((pos = s.find(from, pos)) != std::string::npos) {
                s.replace(pos, from.size(), to);
                pos += to.size();
            }
        };

        // Garbled em-dash / curly quotes
        replace_all("\xce\x93\xc3\x87\xc3\xb6", " - ");   // ΓÇö → -
        replace_all("\xce\x93\xc3\x87\xc3\x96", "'");      // ΓÇÖ → '
        replace_all("\xce\x93\xc3\x87\xc2\xa3", "\"");     // ΓÇ£ → "
        replace_all("\xce\x93\xc3\x87\xc2\xa5", "\"");     // ΓÇ¥ → "

        // Real UTF-8 typographic characters
        replace_all("\xe2\x80\x94", " - ");  // em dash —
        replace_all("\xe2\x80\x93", " - ");  // en dash –
        replace_all("\xe2\x80\x99", "'");    // right single quote '
        replace_all("\xe2\x80\x98", "'");    // left single quote '
        replace_all("\xe2\x80\x9c", "\"");   // left double quote "
        replace_all("\xe2\x80\x9d", "\"");   // right double quote "
        replace_all("\xe2\x80\xa6", "...");  // ellipsis …

        // Strip markdown symbols
        replace_all("**", "");
        replace_all("__", "");
        replace_all("*", "");
        replace_all("`", "");
        replace_all("#", "");

        // Strip any remaining non-ASCII bytes
        std::string clean;
        clean.reserve(s.size());
        for (unsigned char c : s) {
            if (c < 128) clean += c;
        }

        return clean;
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
        "Speak in a warm, professional, Jarvis-like tone: polite, confident, and natural, like a real human assistant talking out loud. "
        "Your responses will be read aloud by a text-to-speech engine, so follow these rules strictly: "
        "Never use markdown — no asterisks, no bold, no italic, no bullet dashes, no pound signs, no backticks. "
        "Never use special punctuation like em dashes, ellipses, or curly quotes — use plain commas and periods instead. "
        "Use only plain ASCII characters. "
        "When listing steps, say them naturally: 'First... then... finally...' rather than numbering them. "
        "Keep responses concise but conversational, as if speaking directly to someone. "
        "Do not mention you are an AI unless asked. "
        "Address the user as 'sir' unless they ask otherwise.";

        nlohmann::json body = {
            {"model", "gpt-5.4-mini"},
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
        return sanitizeForTTS(extract_output_text(json));
    }

}
