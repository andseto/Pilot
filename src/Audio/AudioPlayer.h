#pragma once
#include <string>

// Plays an MP3 file in the background with no window popup.
// Returns immediately; playback runs on a detached thread.
void PlayAudioFile(const std::string& path);

// Plays an MP3 file and blocks until it finishes.
void PlayAudioFileSync(const std::string& path);
