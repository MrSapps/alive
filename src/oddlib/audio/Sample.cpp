#include "oddlib/audio/AliveAudio.h"

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
        return SampleSint16ToFloat(m_SampleBuffer[(int)sampleOffset]); // No interpolation. Faster but sounds jaggy.
    }
    int roundedOffset = (int)floor(sampleOffset);
    return SampleSint16ToFloat(Lerp<Sint16>(m_SampleBuffer[roundedOffset], m_SampleBuffer[roundedOffset + 1], sampleOffset - roundedOffset));
}
