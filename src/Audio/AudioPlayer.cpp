#include "AudioPlayer.h"

#include <windows.h>
#include <mmsystem.h>
#include <thread>
#include <atomic>
#include <string>

static std::atomic<int>  gAudioCounter{0};
static std::atomic<bool> gMusicStop{false};

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

void PlayMusicFile(const std::string& path) {
    // Signal any running music thread to stop, give it time to exit, then start fresh.
    gMusicStop = true;
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    gMusicStop = false;

    std::thread([path]() {
        // Clean up any leftover device from a previous session.
        mciSendStringA("stop pilot_music",  nullptr, 0, nullptr);
        mciSendStringA("close pilot_music", nullptr, 0, nullptr);

        if (mciSendStringA(("open \"" + path + "\" type mpegvideo alias pilot_music").c_str(),
                           nullptr, 0, nullptr) != 0)
            return; // file not found or format error

        mciSendStringA("play pilot_music", nullptr, 0, nullptr);

        // Poll until stop is requested or the track ends naturally.
        while (!gMusicStop) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            char buf[32] = {};
            mciSendStringA("status pilot_music mode", buf, sizeof(buf), nullptr);
            if (std::string(buf) != "playing") break;
        }

        mciSendStringA("stop pilot_music",  nullptr, 0, nullptr);
        mciSendStringA("close pilot_music", nullptr, 0, nullptr);
    }).detach();
}

void StopMusic() {
    gMusicStop = true;
}
