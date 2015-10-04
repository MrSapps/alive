#include "oddlib/audio/AliveAudio.h"
#include "logger.hpp"

template<class T>
static T Lerp(T from, T to, float t)
{
    return from + ((to - from)*t);
}

static float SampleSint16ToFloat(Sint16 v)
{
    return (v / 32767.0f);
}

float AliveAudioSample::GetSample(float sampleOffset, bool interpolation)
{
    if (!interpolation)
    {
        size_t off = (int)sampleOffset;
        if (off >= m_SampleBuffer.size())
        {
            LOG_ERROR("Sample buffer index out of bounds");
            off = m_SampleBuffer.size() - 1;
        }
        return SampleSint16ToFloat(m_SampleBuffer[off]); // No interpolation. Faster but sounds jaggy.
    }
    else
    {
        int roundedOffset = (int)floor(sampleOffset);
        if (roundedOffset+1 >= m_SampleBuffer.size())
        {
            LOG_ERROR("Sample buffer index out of bounds");
            roundedOffset = m_SampleBuffer.size() - 2;
        }
        return SampleSint16ToFloat(Lerp<Sint16>(m_SampleBuffer[roundedOffset], m_SampleBuffer[roundedOffset + 1], sampleOffset - roundedOffset));
    }
}
