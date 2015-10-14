#pragma once

#include "SDL_stdinc.h"
#include "oddlib/audio/AudioInterpolation.h"
#include <vector>

class AliveAudioSample
{
public:
    AliveAudioSample() = default;
    AliveAudioSample(const AliveAudioSample&) = delete;
    AliveAudioSample& operator = (const AliveAudioSample&) = delete;

    std::vector<Uint16> m_SampleBuffer;
    unsigned int mSampleSize = 0;
};
