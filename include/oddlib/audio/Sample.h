#pragma once

#include "SDL_stdinc.h"

class AliveAudioSample
{
public:
    Uint16 * m_SampleBuffer;
    unsigned int i_SampleSize;

    Uint16 GetSample(Uint32 sampleOffset, bool interpolation);
};
