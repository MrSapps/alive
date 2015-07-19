#pragma once

#include "Sample.h"

class AliveAudioTone
{
public:
	// volume 0-1
	float f_Volume;

	// panning -1 - 1
	float f_Pan;

	// Root Key
	unsigned char c_Center;
	unsigned char c_Shift;

	// Key range
	unsigned char Min;
	unsigned char Max;

	float Pitch;

	double AttackTime;
	double ReleaseTime;
	bool ReleaseExponential = false;
	double DecayTime;
	double SustainTime;

	bool Loop = false;

	AliveAudioSample * m_Sample;
};