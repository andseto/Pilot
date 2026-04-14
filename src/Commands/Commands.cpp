#include "Commands.h"

#include <string>
#include <vector>
#include <functional>
#include <algorithm>
#include <cctype>
#include <windows.h>
#include <shlobj.h>
#include <iostream>
#include <nlohmann/json.hpp>
#include "../Audio/AudioPlayer.h"

// ============================================================
//  Helpers
// ============================================================

static std::string toLower(const std::string& s) {
    std::string r = s;
    for (char& c : r) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return r;
}

// Opens a URL in the default browser (no cmd window)
static void OpenURL(const std::string& url) {
    ShellExecuteA(nullptr, "open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

// ============================================================
//  Command Definitions
//  To add a new command:
//    1. Write a static void function below
//    2. Add an entry to the `commands` table in TryHandle()
// ============================================================

static void CMD_PlayWakeMeUp() {
    PlayMusicFile("src/music/WakeUp.mp3");
}

static void CMD_PlayLostTrade() {
    PlayMusicFile("src/music/LostATrade.mp3");
}

static void CMD_PlayWonTrade() {
    PlayMusicFile("src/music/WonATrade.mp3");
}

static void CMD_StopMusic() {
    StopMusic();
}

static void CMD_OpenTradingSetup() {
    std::cout << "[Pilot] Opening trading setup...\n";

    // Add your trading URLs here
    OpenURL("https://www.tradingview.com");
    OpenURL("https://app.tradesyncer.com/cockpit");
    OpenURL("https://www.forexfactory.com");
    OpenURL("https://finance.yahoo.com");
}

// ============================================================
//  Command Table
// ============================================================

struct Command {
    std::vector<std::string> triggers;   // phrases that activate this command
    std::function<void()>    action;
    std::string              reply;      // what Pilot says after running it
};

static const std::vector<Command> commands = {

    {
        { "open trading setup", "trading setup", "open my trading setup" },
        CMD_OpenTradingSetup,
        "Opening your trading setup now, sir."
    },

    {
        { "wake me up", "pilot wake me up" },
        CMD_PlayWakeMeUp,
        "Let's go, sir."
    },

    {
        { "i lost a trade", "pilot i lost a trade" },
        CMD_PlayLostTrade,
        "Shake it off, sir."
    },

    {
        { "i won a trade", "pilot i won a trade" },
        CMD_PlayWonTrade,
        "That's what I'm talking about, sir."
    },

    {
        { "stop music", "pilot stop music", "stop the music", "pilot stop the music",
          "pause music", "pause the music", "kill music", "kill the music",
          "turn off music", "turn off the music" },
        CMD_StopMusic,
        "Music stopped, sir."
    },

    // ── Add new commands below this line ─────────────────────────────────
    //
    // {
    //     { "trigger phrase one", "trigger phrase two" },
    //     CMD_YourFunction,
    //     "What Pilot says after running it."
    // },

};

// ============================================================
//  Dispatcher
// ============================================================

namespace Commands {

    bool TryHandle(const std::string& text) {
        const std::string lower = toLower(text);

        for (const auto& cmd : commands) {
            for (const auto& trigger : cmd.triggers) {
                if (lower.find(trigger) != std::string::npos) {
                    std::cout << "[Pilot] Command matched: \"" << trigger << "\"\n";
                    cmd.action();
                    std::cout << "Pilot: " << cmd.reply << "\n";
                    return true;
                }
            }
        }

        return false; // no command matched
    }

}

// ============================================================
//  Tool Execution (ChatGPT function calling)
// ============================================================

// Resolves a spoken location name to an absolute folder path.
static std::string GetKnownFolderPath(const std::string& location) {
    KNOWNFOLDERID id = FOLDERID_Desktop;
    if (location == "documents") id = FOLDERID_Documents;
    else if (location == "downloads") id = FOLDERID_Downloads;
    // default: desktop

    PWSTR wide = nullptr;
    if (FAILED(SHGetKnownFolderPath(id, 0, nullptr, &wide)) || !wide)
        return "";

    // Convert wide → UTF-8
    int len = WideCharToMultiByte(CP_UTF8, 0, wide, -1, nullptr, 0, nullptr, nullptr);
    std::string result(len - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide, -1, &result[0], len, nullptr, nullptr);
    CoTaskMemFree(wide);
    return result;
}

static std::string Tool_CreateFolder(const nlohmann::json& args) {
    std::string name     = args.value("name", "");
    std::string location = args.value("location", "desktop");

    if (name.empty())
        return "I need a folder name, sir.";

    std::string base = GetKnownFolderPath(location);
    if (base.empty())
        return "I could not resolve that location, sir.";

    std::string full = base + "\\" + name;

    if (CreateDirectoryA(full.c_str(), nullptr))
        return "Done, sir. I created the " + name + " folder on your " + location + ".";

    if (GetLastError() == ERROR_ALREADY_EXISTS)
        return "That folder already exists on your " + location + ", sir.";

    return "I was unable to create the folder, sir. Access may be restricted.";
}

static std::string Tool_OpenApplication(const nlohmann::json& args) {
    std::string app = args.value("app_name", "");
    if (app.empty())
        return "Which application should I open, sir?";

    HINSTANCE h = ShellExecuteA(nullptr, "open", app.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    if (reinterpret_cast<intptr_t>(h) > 32)
        return "Opening " + app + " now, sir.";

    return "I could not open " + app + ", sir. It may not be installed or the name may be incorrect.";
}

static std::string Tool_OpenURL(const nlohmann::json& args) {
    std::string url = args.value("url", "");
    if (url.empty())
        return "I need a URL to open, sir.";

    ShellExecuteA(nullptr, "open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    return "Opening that in your browser now, sir.";
}

namespace Commands {

    std::string ExecuteTool(const std::string& name, const std::string& argsJson) {
        nlohmann::json args;
        try {
            args = nlohmann::json::parse(argsJson);
        } catch (...) {
            args = nlohmann::json::object();
        }

        std::cout << "[Pilot] Tool call: " << name << " " << argsJson << "\n";

        if (name == "create_folder")    return Tool_CreateFolder(args);
        if (name == "open_application") return Tool_OpenApplication(args);
        if (name == "open_url")         return Tool_OpenURL(args);

        return "I received an unknown command, sir.";
    }

}
