#pragma once

#include "SDL.h"

#include <stdio.h>
#include <iostream>
#include <vector>
#include <algorithm>

#include "vab.hpp"
#include "oddlib/lvlarchive.hpp"
#include "jsonxx/jsonxx.h"

#include "Soundbank.h"
#include "Program.h"
#include "Tone.h"
#include "Voice.h"
#include "Sample.h"
#include "ADSR.h"
#include "biquad.h"
#include "core/audiobuffer.hpp"
#include "stdthread.h"

const int AliveAudioSampleRate = 44100;

class FileSystem;

class AliveAudio : public IAudioPlayer
{
public:
    std::vector<unsigned char> m_SoundsDat;
    void PlayOneShot(int program, int note, float volume, float pitch = 0);
    void PlayOneShot(std::string soundID);

    void NoteOn(int program, int note, char velocity, float pitch = 0, int trackID = 0, float trackDelay = 0);
    void NoteOn(int program, int note, char velocity, int trackID = 0, float trackDelay = 0);

    void NoteOff(int program, int note, int trackID = 0);
    void NoteOffDelay(int program, int note, int trackID = 0, float trackDelay = 0);

    void DebugPlayFirstToneSample(int program, int tone);

    void ClearAllVoices(bool forceKill = true);
    void ClearAllTrackVoices(int trackID, bool forceKill = false);

    void LoadSoundbank(char * fileName);
    void SetSoundbank(AliveAudioSoundbank * soundbank);

    void LoadAllFromLvl(std::string lvlPath, std::string vabID, std::string seqFile, FileSystem& fs);
    void LoadAllFromLvl(Oddlib::LvlArchive& lvlArchive, std::string vabID, std::string seqFile);

    biquad * AliveAudioEQBiQuad = nullptr;
    std::mutex EQMutex;

    AliveAudioSoundbank* m_CurrentSoundbank = nullptr;
    std::vector<std::vector<Uint8>> m_LoadedSeqData;
    std::recursive_mutex voiceListMutex;
    std::vector<AliveAudioVoice *> m_Voices;
    bool Interpolation = true;
    bool EQEnabled = false;
    Uint64 currentSampleIndex = 0;
    jsonxx::Object m_Config;

    void AliveAudioSetEQ(float cutoff)
    {
        
        EQMutex.lock();

        if (AliveAudio::AliveAudioEQBiQuad != nullptr)
            delete AliveAudioEQBiQuad;

        AliveAudioEQBiQuad = BiQuad_new(PEQ, 8u, cutoff, AliveAudioSampleRate, 1u);
        EQMutex.unlock();
       
    }

    void AliveEQEffect(float* stream, int len)
    {
       
        if (AliveAudioEQBiQuad == nullptr)
        {
            AliveAudioSetEQ(20500);
        }

        EQMutex.lock();

        for (int i = 0; i < len; i++)
        {
            stream[i] = BiQuad(stream[i], AliveAudioEQBiQuad);
        }

        EQMutex.unlock();
    }

    virtual void Play(Uint8* stream, Uint32 len) override;
    void AliveInitAudio(FileSystem& fs);
private:
    void CleanVoices();
    void AliveRenderAudio(float* AudioStream, int StreamLength);

    void LoadJsonConfig(std::string filePath, FileSystem& fs);
};

///////////////
