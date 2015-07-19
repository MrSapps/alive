#pragma once

static class AliveAudioHelper
{
public:

	template<class T>
	static T Lerp(T from, T to, float t)
	{
		return from + ((to - from)*t);
	}

	static float SampleSint16ToFloat(Sint16 v)
	{
		return (v / 32767.0f);
	}

	static float RandFloat(float a, float b)
	{
		return ((b - a)*((float)rand() / RAND_MAX)) + a;
	}
};