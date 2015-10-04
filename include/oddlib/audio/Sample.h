#pragma once

#include "SDL_stdinc.h"
#include <vector>

class AliveAudioSample
{
public:
    std::vector<Uint16> m_SampleBuffer;
    unsigned int i_SampleSize;

    float GetSample(float sampleOffset, bool interpolation);
};
