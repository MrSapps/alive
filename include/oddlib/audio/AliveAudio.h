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
#include "Voice.h"
#include "Sample.h"
#include "ADSR.h"
#include "core/audiobuffer.hpp"
#include "stdthread.h"
#include "AudioInterpolation.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4267) //  'return' : conversion from 'size_t' to 'unsigned long', possible loss of data
#endif
#include "stk/include/FreeVerb.h"
#ifdef _MSC_VER
#pragma warning(pop)
#endif

const int AliveAudioSampleRate = 44100;

class FileSystem;

class AliveAudio : public IAudioPlayer
{
public:
    std::vector<unsigned char> m_SoundsDat;
    void PlayOneShot(int program, int note, float volume, float pitch = 0);
    void PlayOneShot(std::string soundID);

    void NoteOn(int program, int note, char velocity, float pitch = 0, int trackID = 0, double trackDelay = 0);
    void NoteOn(int program, int note, char velocity, int trackID = 0, double trackDelay = 0);

    void NoteOff(int program, int note, int trackID = 0);
    void NoteOffDelay(int program, int note, int trackID = 0, float trackDelay = 0);

    void DebugPlayFirstToneSample(int program, int tone);

    void ClearAllVoices(bool forceKill = true);
    void ClearAllTrackVoices(int trackID, bool forceKill = false);

    void LoadSoundbank(char * fileName);
    void SetSoundbank(std::unique_ptr<AliveAudioSoundbank> soundbank);

    void LoadAllFromLvl(std::string lvlPath, std::string vabID, std::string seqFile, FileSystem& fs);
    void LoadAllFromLvl(Oddlib::LvlArchive& lvlArchive, std::string vabID, std::string seqFile);


    std::vector<std::vector<Uint8>> m_LoadedSeqData;
    std::recursive_mutex voiceListMutex;
    jsonxx::Object m_Config;
    Uint64 currentSampleIndex = 0;

    virtual void Play(Uint8* stream, Uint32 len) override;
    void AliveInitAudio(FileSystem& fs);

    // Can be changed from outside class
    AudioInterpolation Interpolation = AudioInterpolation_hermite;
    bool AntiAliasFilteringEnabled = false;
    bool ForceReverb = false;
    float ReverbMix = 0.5f;
    bool DebugDisableVoiceResampling = false;

private:
    std::unique_ptr<AliveAudioSoundbank> m_CurrentSoundbank;

    std::vector<AliveAudioVoice *> m_Voices;
    std::vector<float> m_DryChannelBuffer;
    std::vector<float> m_ReverbChannelBuffer;

    stk::FreeVerb m_Reverb;

    void CleanVoices();
    void AliveRenderAudio(float* AudioStream, int StreamLength);

    void LoadJsonConfig(std::string filePath, FileSystem& fs);
};

///////////////
