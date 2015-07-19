#pragma once

#include "SDL.h"

#include <stdio.h>
#include <iostream>
#include <vector>
#include <mutex>
#include <algorithm>

#include "vab.hpp"
#include "oddlib/lvlarchive.hpp"
#include "jsonxx/jsonxx.h"

#include "Soundbank.h"
#include "Helper.h"
#include "Program.h"
#include "Tone.h"
#include "Voice.h"
#include "Sample.h"
#include "ADSR.h"
#include "biquad.h"

void AliveInitAudio();
void AliveAudioSDLCallback(void *udata, Uint8 *stream, int len);

const int AliveAudioSampleRate = 44100;

class AliveAudio
{
public:
	static std::vector<unsigned char> m_SoundsDat;
	static void PlayOneShot(int program, int note, float volume, float pitch = 0);
	static void PlayOneShot(std::string soundID);

	static void NoteOn(int program, int note, char velocity, float pitch = 0, int trackID = 0, float trackDelay = 0);
	static void NoteOn(int program, int note, char velocity, int trackID = 0, float trackDelay = 0);

	static void NoteOff(int program, int note, int trackID = 0);
	static void NoteOffDelay(int program, int note, int trackID = 0, float trackDelay = 0);

	static void DebugPlayFirstToneSample(int program, int tone);

	static void LockNotes();
	static void UnlockNotes();

	static void ClearAllVoices(bool forceKill = true);
	static void ClearAllTrackVoices(int trackID, bool forceKill = false);

	static void LoadSoundbank(char * fileName);
	static void SetSoundbank(AliveAudioSoundbank * soundbank);

	static void LoadAllFromLvl(std::string lvlPath, std::string vabID, std::string seqFile);
	static void LoadAllFromLvl(Oddlib::LvlArchive& lvlArchive, std::string vabID, std::string seqFile);

	static biquad * AliveAudioEQBiQuad;
	static std::mutex EQMutex;

	static AliveAudioSoundbank* m_CurrentSoundbank;
	static std::vector<std::vector<Uint8>> m_LoadedSeqData;
	static std::mutex voiceListMutex;
	static std::vector<AliveAudioVoice *> m_Voices;
	static bool Interpolation;
	static bool EQEnabled;
	static bool voiceListLocked;
	static long long currentSampleIndex;
	static jsonxx::Object m_Config;
};

static void AliveAudioSetEQ(float cutoff)
{
	AliveAudio::EQMutex.lock();

	if (AliveAudio::AliveAudioEQBiQuad != nullptr)
		delete AliveAudio::AliveAudioEQBiQuad;

	AliveAudio::AliveAudioEQBiQuad = BiQuad_new(PEQ, 8, cutoff, AliveAudioSampleRate, 1);
	AliveAudio::EQMutex.unlock();
}

static void AliveEQEffect(float * stream, int len)
{
	if (AliveAudio::AliveAudioEQBiQuad == nullptr)
		AliveAudioSetEQ(20500);

	AliveAudio::EQMutex.lock();

	for (int i = 0; i < len; i++)
	{
		stream[i] = BiQuad(stream[i], AliveAudio::AliveAudioEQBiQuad);
	}

	AliveAudio::EQMutex.unlock();
}
///////////////