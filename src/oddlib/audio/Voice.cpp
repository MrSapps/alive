#include "oddlib/audio/Voice.h"
#include "oddlib/audio/AliveAudio.h"

float AliveAudioVoice::GetSample(bool interpolation)
{
    if (b_Dead)	// Don't return anything if dead. This voice should now be removed.
    {
        return 0;
    }

    // ActiveDecayLevel -= ((1.0 / AliveAudioSampleRate) / m_Tone->DecayTime);

    if (ActiveDecayLevel < 0)
    {
        ActiveDecayLevel = 0;
    }

    if (!b_NoteOn)
    {
        ActiveReleaseLevel -= ((1.0 / AliveAudioSampleRate) / m_Tone->ReleaseTime);

        ActiveReleaseLevel -= ((1.0 / AliveAudioSampleRate) / m_Tone->DecayTime); // Hacks?!?! --------------------

        if (ActiveReleaseLevel <= 0) // Release is done. So the voice is done.
        {
            b_Dead = true;
            ActiveReleaseLevel = 0;
        }
    }
    else
    {
        ActiveAttackLevel += ((1.0 / AliveAudioSampleRate) / m_Tone->AttackTime);

        if (ActiveAttackLevel > 1)
        {
            ActiveAttackLevel = 1;
        }
    }

	// That constant is 2^(1/12)
	double sampleFrameRate = pow(1.05946309436, i_Note - m_Tone->c_Center + m_Tone->Pitch + f_Pitch) * (44100.0 / AliveAudioSampleRate);
	f_SampleOffset += (sampleFrameRate);

    // For some reason, for samples that dont loop, they need to be cut off 1 sample earlier.
    // Todo: Revise this. Maybe its the loop flag at the end of the sample!? 
    if ((m_Tone->Loop) ? f_SampleOffset >= m_Tone->m_Sample->mSampleSize : (f_SampleOffset >= m_Tone->m_Sample->mSampleSize - 1))
    {
        if (m_Tone->Loop)
        {
			while (f_SampleOffset >= m_Tone->m_Sample->mSampleSize)
	            f_SampleOffset -= m_Tone->m_Sample->mSampleSize;
        }
        else
        {
            b_Dead = true;
            return 0; // Voice is dead now, so don't return anything.
        }
    }

    return m_Tone->m_Sample->GetSample(f_SampleOffset, interpolation) * ActiveAttackLevel * ActiveDecayLevel * ((b_NoteOn) ? ActiveSustainLevel : ActiveReleaseLevel) * f_Velocity;
}
