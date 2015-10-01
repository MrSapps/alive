#include "oddlib/audio/AliveAudio.h"

Uint16 AliveAudioSample::GetSample(Uint32 sampleOffset, bool interpolation)
{
    // TODO FIX ME
    //if (!interpolation)
    {
        return AliveAudioHelper::SampleSint16ToFloat(m_SampleBuffer[sampleOffset]); // No interpolation. Faster but sounds jaggy.
    }
   // int roundedOffset = (int)floor(sampleOffset);
   // return AliveAudioHelper::SampleSint16ToFloat(AliveAudioHelper::Lerp<Sint16>(m_SampleBuffer[roundedOffset], m_SampleBuffer[roundedOffset + 1], sampleOffset - roundedOffset));
}
