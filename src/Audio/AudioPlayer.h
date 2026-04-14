#pragma once
#include <string>

// Plays an MP3 file in the background with no window popup.
// Returns immediately; playback runs on a detached thread.
void PlayAudioFile(const std::string& path);

// Plays an MP3 file and blocks until it finishes.
void PlayAudioFileSync(const std::string& path);

// Plays a music file in the background using a fixed alias.
// Stops and replaces any currently playing music track.
void PlayMusicFile(const std::string& path);

// Stops any music started by PlayMusicFile.
void StopMusic();
