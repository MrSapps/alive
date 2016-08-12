#include "oddlib/audio/Voice.h"
#include "oddlib/audio/AliveAudio.h"
#include "oddlib/audio/Sample.h"
#include "logger.hpp"

#include <algorithm>

template<class T>
static float Lerp(T from, T to, float t)
{
    return from + ((to - from)*t);
}

static float SampleSint16ToFloat(s16 v)
{
    return (v / 32767.0f);
}

float InterpCubic(float x0, float x1, float x2, float x3, float t)
{
    float a0, a1, a2, a3;
    a0 = x3 - x2 - x0 + x1;
    a1 = x0 - x1 - a0;
    a2 = x2 - x0;
    a3 = x1;
    return a0*(t*t*t) + a1*(t*t) + a2*t + a3;
}

float InterpHermite(float x0, float x1, float x2, float x3, float t)
{
    float c0 = x1;
    float c1 = .5F * (x2 - x0);
    float c2 = x0 - (2.5F * x1) + (2 * x2) - (.5F * x3);
    float c3 = (.5F * (x3 - x0)) + (1.5F * (x1 - x2));
    return (((((c3 * t) + c2) * t) + c1) * t) + c0;
}

float AliveAudioVoice::GetSample(AudioInterpolation interpolation, bool /*antialiasFilteringEnabled*/)
{
	if (b_Dead)	// Don't return anything if dead. This voice should now be removed.
	{
		return 0;
	}

    if (m_ADSR_State == ADSR_State_attack)
    {
        if (b_NoteOn)
        {
            m_ADSR_Level += ((1.0 / AliveAudioSampleRate) / m_Tone->Env.AttackTime);
            if (m_ADSR_Level > 1.0)
            {
                m_ADSR_Level = 1.0;
                m_ADSR_State = ADSR_State_decay;
            }
        }
        else
        {
            m_ADSR_State = ADSR_State_release;
        }
    }
    else if (m_ADSR_State == ADSR_State_decay)
    {
        if (b_NoteOn)
        {
            if (m_Tone->Env.DecayTime > 0.0)
                m_ADSR_Level -= ((1.0 / AliveAudioSampleRate) / m_Tone->Env.DecayTime);

            if (m_Tone->Env.DecayTime <= 0.0 || m_ADSR_Level < m_Tone->Env.SustainLevel)
            {
                m_ADSR_Level = m_Tone->Env.SustainLevel;
                m_ADSR_State = ADSR_State_sustain;
            }
        }
        else
        {
            m_ADSR_State = ADSR_State_release;
        }
    }
    else if (m_ADSR_State == ADSR_State_sustain)
    {
        if (!b_NoteOn)
            m_ADSR_State = ADSR_State_release;
    }
    else if (m_ADSR_State == ADSR_State_release)
    {
        if (m_Tone->Env.ExpRelease)
        {
            double delta = m_ADSR_Level*((1.0 / AliveAudioSampleRate) / m_Tone->Env.LinearReleaseTime); // Exp starts as fast as linear
            if (delta < 0.000001)
                delta = 0.000001; // Avoid denormals, and make sure that the voice ends some day
            m_ADSR_Level -= delta;
        }
        else
        {
            m_ADSR_Level -= ((1.0 / AliveAudioSampleRate) / m_Tone->Env.LinearReleaseTime);
        }
    }

    if (m_ADSR_Level <= 0) // Release/decay is done. So the voice is done.
    {
        b_Dead = true;
        m_ADSR_Level = 0.0f;
    }

    // That constant is 2^(1/12)
    double sampleFrameRateMul = pow(1.05946309436, i_Note - m_Tone->c_Center + m_Tone->Pitch + f_Pitch) * (44100.0 / AliveAudioSampleRate);
    if (m_DebugDisableResampling)
        sampleFrameRateMul = 1.0f;
    f_SampleOffset += (sampleFrameRateMul);

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

    { // Actual sample calculation
        std::vector<u16>& sampleBuffer = m_Tone->m_Sample->m_SampleBuffer;

        float sample = 0.0f;
        if (interpolation == AudioInterpolation_none)
        {
            size_t off = (int)f_SampleOffset;
            if (off >= sampleBuffer.size())
            {
                LOG_ERROR("Sample buffer index out of bounds");
                off = sampleBuffer.size() - 1;
            }
            sample = SampleSint16ToFloat(sampleBuffer[off]); // No interpolation. Faster but sounds jaggy.
        }
        else if (interpolation == AudioInterpolation_linear)
        {
            int baseOffset = static_cast<int>(floor(f_SampleOffset));
            int nextOffset = (baseOffset + 1) % sampleBuffer.size(); // TODO: Don't assume looping
            if (baseOffset >= static_cast<int>(sampleBuffer.size()))
            {
                LOG_ERROR("Sample buffer index out of bounds (interpolated)");
                baseOffset = nextOffset = static_cast<int>(sampleBuffer.size() - 1);
            }
            sample = SampleSint16ToFloat(static_cast<s16>(Lerp<s16>(sampleBuffer[baseOffset], sampleBuffer[nextOffset], static_cast<float>(f_SampleOffset - baseOffset))));
        }
        else if (interpolation == AudioInterpolation_cubic || interpolation == AudioInterpolation_hermite)
        {
            int offsets[4] = { (int)floor(f_SampleOffset) - 1, 0, 0, 0 };
            if (offsets[0] < 0)
                offsets[0] += static_cast<int>(sampleBuffer.size());
            for (int i = 1; i < 4; ++i)
            {
                offsets[i] = (offsets[i - 1] + 1) % sampleBuffer.size(); // TODO: Don't assume looping
            }
            float raw_samples[4] =
            {
                SampleSint16ToFloat(sampleBuffer[offsets[0]]),
                SampleSint16ToFloat(sampleBuffer[offsets[1]]),
                SampleSint16ToFloat(sampleBuffer[offsets[2]]),
                SampleSint16ToFloat(sampleBuffer[offsets[3]]),
            };

            float t = static_cast<float>(f_SampleOffset - floor(f_SampleOffset));
            assert(t >= 0.0 && t <= 1.0);
            if (interpolation == AudioInterpolation_cubic)
                sample = InterpCubic(raw_samples[0], raw_samples[1], raw_samples[2], raw_samples[3], t);
            else 
                sample = InterpHermite(raw_samples[0], raw_samples[1], raw_samples[2], raw_samples[3], t);
        }

        return static_cast<float>(sample * m_ADSR_Level * f_Velocity);
    }
}
