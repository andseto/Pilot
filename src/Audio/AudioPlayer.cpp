#include "AudioPlayer.h"

#include <windows.h>
#include <mmsystem.h>
#include <thread>
#include <atomic>
#include <string>

static std::atomic<int> gAudioCounter{0};

void PlayAudioFile(const std::string& path) {
    std::thread([path]() {
        std::string alias = "pilot_audio_" + std::to_string(gAudioCounter++);
        mciSendStringA(("open \"" + path + "\" type mpegvideo alias " + alias).c_str(), nullptr, 0, nullptr);
        mciSendStringA(("play " + alias + " wait").c_str(),                             nullptr, 0, nullptr);
        mciSendStringA(("close " + alias).c_str(),                                      nullptr, 0, nullptr);
    }).detach();
}

void PlayAudioFileSync(const std::string& path) {
    std::string alias = "pilot_audio_" + std::to_string(gAudioCounter++);
    mciSendStringA(("open \"" + path + "\" type mpegvideo alias " + alias).c_str(), nullptr, 0, nullptr);
    mciSendStringA(("play " + alias + " wait").c_str(),                             nullptr, 0, nullptr);
    mciSendStringA(("close " + alias).c_str(),                                      nullptr, 0, nullptr);
}
