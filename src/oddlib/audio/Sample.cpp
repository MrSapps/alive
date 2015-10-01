#include "oddlib/audio/AliveAudio.h"

float AliveAudioSample::GetSample(float sampleOffset, bool interpolation)
{
    if (!interpolation)
    {
        return AliveAudioHelper::SampleSint16ToFloat(m_SampleBuffer[(int)sampleOffset]); // No interpolation. Faster but sounds jaggy.
    }
    int roundedOffset = (int)floor(sampleOffset);
    return AliveAudioHelper::SampleSint16ToFloat(AliveAudioHelper::Lerp<Sint16>(m_SampleBuffer[roundedOffset], m_SampleBuffer[roundedOffset + 1], sampleOffset - roundedOffset));
}
