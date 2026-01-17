#include "AudioCapture.h"
#include <portaudio.h>

AudioCapture::AudioCapture(int sampleRate, int framesPerBuffer)
    : sampleRate_(sampleRate), framesPerBuffer_(framesPerBuffer), stream_(nullptr)
{
    Pa_Initialize();
}

AudioCapture::~AudioCapture()
{
    stop();
    Pa_Terminate();
}

bool AudioCapture::start()
{
    PaStream* s = nullptr;
    PaError err = Pa_OpenDefaultStream(
        &s,
        1,
        0,
        paInt16,
        sampleRate_,
        framesPerBuffer_,
        nullptr,
        nullptr
    );
    if (err != paNoError) return false;

    err = Pa_StartStream(s);
    if (err != paNoError) { Pa_CloseStream(s); return false; }

    stream_ = s;
    return true;
}

void AudioCapture::stop()
{
    if (!stream_) return;
    auto* s = reinterpret_cast<PaStream*>(stream_);
    Pa_StopStream(s);
    Pa_CloseStream(s);
    stream_ = nullptr;
}

bool AudioCapture::read(std::vector<int16_t>& out)
{
    if (!stream_) return false;
    out.resize(framesPerBuffer_);

    auto* s = reinterpret_cast<PaStream*>(stream_);
    PaError err = Pa_ReadStream(s, out.data(), framesPerBuffer_);
    return err == paNoError;
}
