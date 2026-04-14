#include "ConversationMemory.h"

#include <windows.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <ctime>

// Max number of past exchanges to send to the API (each = 1 user + 1 assistant message)
static const int MAX_HISTORY_EXCHANGES = 10;

// ── Path helpers ─────────────────────────────────────────────────────────────

static std::string getPrimaryDrive() {
    char winDir[MAX_PATH];
    GetWindowsDirectoryA(winDir, MAX_PATH);
    // winDir looks like "C:\Windows" — take just "C:\"
    return std::string(winDir).substr(0, 3);
}

static std::string getTodayLabel() {
    time_t t = time(nullptr);
    struct tm tm{};
    localtime_s(&tm, &t);

    const char* months[] = {
        "January","February","March","April","May","June",
        "July","August","September","October","November","December"
    };
    char buf[64];
    snprintf(buf, sizeof(buf), "%s%02d-%04d",
             months[tm.tm_mon], tm.tm_mday, tm.tm_year + 1900);
    return buf;
}

std::string ConversationMemory::buildFilePath() {
    std::string dir = getPrimaryDrive() + "ChatGPT_API_Memory";

    // Create the folder if it doesn't exist
    CreateDirectoryA(dir.c_str(), nullptr);

    return dir + "\\" + getTodayLabel() + ".txt";
}

// ── Constructor ───────────────────────────────────────────────────────────────

ConversationMemory::ConversationMemory() {
    filePath_ = buildFilePath();
    loadFromFile();
    std::cout << "[Memory] Using file: " << filePath_ << "\n";
    std::cout << "[Memory] Loaded " << (history_.size() / 2) << " past exchange(s).\n";
}

// ── Load ──────────────────────────────────────────────────────────────────────

void ConversationMemory::loadFromFile() {
    std::ifstream f(filePath_);
    if (!f.is_open()) return;

    std::string line;
    while (std::getline(f, line)) {
        if (line.rfind("USER: ", 0) == 0) {
            history_.push_back({"user",      line.substr(6)});
        } else if (line.rfind("ASSISTANT: ", 0) == 0) {
            history_.push_back({"assistant", line.substr(11)});
        }
    }

    // Trim to the most recent MAX_HISTORY_EXCHANGES exchanges (2 messages each)
    const int maxMessages = MAX_HISTORY_EXCHANGES * 2;
    if ((int)history_.size() > maxMessages) {
        history_.erase(history_.begin(),
                       history_.begin() + ((int)history_.size() - maxMessages));
    }
}

// ── Persist ───────────────────────────────────────────────────────────────────

void ConversationMemory::appendToFile(const std::string& userText,
                                      const std::string& assistantText) {
    std::ofstream f(filePath_, std::ios::app);
    if (!f.is_open()) {
        std::cerr << "[Memory] Could not write to " << filePath_ << "\n";
        return;
    }
    f << "USER: "      << userText      << "\n";
    f << "ASSISTANT: " << assistantText << "\n";
}

// ── Public API ────────────────────────────────────────────────────────────────

void ConversationMemory::add(const std::string& userText,
                             const std::string& assistantText) {
    history_.push_back({"user",      userText});
    history_.push_back({"assistant", assistantText});

    // Keep in-memory list trimmed
    const int maxMessages = MAX_HISTORY_EXCHANGES * 2;
    if ((int)history_.size() > maxMessages) {
        history_.erase(history_.begin(),
                       history_.begin() + ((int)history_.size() - maxMessages));
    }

    appendToFile(userText, assistantText);
}

const std::vector<Message>& ConversationMemory::getHistory() const {
    return history_;
}

std::string ConversationMemory::getFilePath() const {
    return filePath_;
}
