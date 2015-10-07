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

float AliveAudioSample::GetSample(double sampleOffset, bool interpolation)
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
		int baseOffset = (int)floor(sampleOffset);
		int nextOffset = (baseOffset + 1) % m_SampleBuffer.size(); // TODO: Don't assume looping
		if (baseOffset >= m_SampleBuffer.size())
		{
            LOG_ERROR("Sample buffer index out of bounds (interpolated)");
            baseOffset = nextOffset = m_SampleBuffer.size() - 1;
		}
        return SampleSint16ToFloat(Lerp<Sint16>(m_SampleBuffer[baseOffset], m_SampleBuffer[nextOffset], sampleOffset - baseOffset));
    }
}
