#include "AudioCapture.h"
#include <portaudio.h>
#include <iostream>

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

bool AudioCapture::start(int deviceIndex)
{
    PaDeviceIndex device = (deviceIndex >= 0) ? deviceIndex : Pa_GetDefaultInputDevice();
    const PaDeviceInfo* info = Pa_GetDeviceInfo(device);

    PaStreamParameters inputParams;
    inputParams.device                    = device;
    inputParams.channelCount              = 1;
    inputParams.sampleFormat              = paInt16;
    inputParams.suggestedLatency          = info ? info->defaultLowInputLatency : 0.1;
    inputParams.hostApiSpecificStreamInfo = nullptr;

    PaStream* s = nullptr;
    PaError err = Pa_OpenStream(
        &s,
        &inputParams,
        nullptr,
        sampleRate_,
        framesPerBuffer_,
        paClipOff,
        nullptr,
        nullptr
    );
    if (err != paNoError) {
        std::cerr << "[AudioCapture] Failed to open mic: " << Pa_GetErrorText(err) << "\n";
        return false;
    }

    err = Pa_StartStream(s);
    if (err != paNoError) {
        std::cerr << "[AudioCapture] Failed to start stream: " << Pa_GetErrorText(err) << "\n";
        Pa_CloseStream(s);
        return false;
    }

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
