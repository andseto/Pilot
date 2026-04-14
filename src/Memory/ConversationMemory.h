#pragma once
#include <string>
#include <vector>

struct Message {
    std::string role;     // "user" or "assistant"
    std::string content;
};

class ConversationMemory {
public:
    ConversationMemory();

    // Record a completed exchange and persist it to today's file
    void add(const std::string& userText, const std::string& assistantText);

    // Returns the loaded history for inclusion in the next API call
    const std::vector<Message>& getHistory() const;

    // Path to today's memory file (for display/debug)
    std::string getFilePath() const;

private:
    std::vector<Message> history_;
    std::string          filePath_;

    void        loadFromFile();
    void        appendToFile(const std::string& userText, const std::string& assistantText);
    static std::string buildFilePath();
};
