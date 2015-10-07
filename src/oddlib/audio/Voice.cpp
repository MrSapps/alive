#include "oddlib/audio/Voice.h"
#include "oddlib/audio/AliveAudio.h"
#include "oddlib/audio/Sample.h"
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

float InterpCubic(float x0, float x1, float x2, float x3, float t)
{
	float a0, a1, a2, a3;
	a0 = x3 - x2 - x0 + x1;
	a1 = x0 - x1 - a0;
	a2 = x2 - x0;
	a3 = x1;
	return a0*(t*t*t) + a1*(t*t) + a2*t + a3;
}

float AliveAudioVoice::GetSample(AudioInterpolation interpolation, bool antialiasFilteringEnabled)
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
	double sampleFrameRateMul = pow(1.05946309436, i_Note - m_Tone->c_Center + m_Tone->Pitch + f_Pitch) * (44100.0 / AliveAudioSampleRate);
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
		std::vector<Uint16>& sampleBuffer = m_Tone->m_Sample->m_SampleBuffer;

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
			int baseOffset = (int)floor(f_SampleOffset);
			int nextOffset = (baseOffset + 1) % sampleBuffer.size(); // TODO: Don't assume looping
			if (baseOffset >= sampleBuffer.size())
			{
				LOG_ERROR("Sample buffer index out of bounds (interpolated)");
				baseOffset = nextOffset = sampleBuffer.size() - 1;
			}
			sample = SampleSint16ToFloat(Lerp<Sint16>(sampleBuffer[baseOffset], sampleBuffer[nextOffset], f_SampleOffset - baseOffset));
		}
		else if (interpolation == AudioInterpolation_cubic)
		{
			int offsets[4] = { (int)floor(f_SampleOffset) - 1, 0, 0, 0 };
			if (offsets[0] < 0)
				offsets[0] += sampleBuffer.size();
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

			float t = f_SampleOffset - floor(f_SampleOffset);
			assert(t >= 0.0 && t <= 1.0);
			sample = InterpCubic(raw_samples[0], raw_samples[1], raw_samples[2], raw_samples[3], t);
		}

		if (antialiasFilteringEnabled)
		{
			if (sampleFrameRateMul > 1.0f) // Aliasing will occur only when speeding up
				sample = (m_LastSample + sample)/2; // Really, really basic way to reduce frequencies above nyquist. TODO: This should be before interpolation (?).
			m_LastSample = sample;
		}

		return sample * ActiveAttackLevel * ActiveDecayLevel * ((b_NoteOn) ? ActiveSustainLevel : ActiveReleaseLevel) * f_Velocity;
	}
}
