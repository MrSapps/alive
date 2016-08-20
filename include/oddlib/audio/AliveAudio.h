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
    //void PlayOneShot(int program, int note, f32 volume, f32 pitch = 0);
    //void PlayOneShot(std::string soundID);

    void NoteOn(int program, int note, char velocity, f32 pitch = 0, int trackID = 0, f64 trackDelay = 0);
    void NoteOn(int program, int note, char velocity, int trackID = 0, f64 trackDelay = 0);

    void NoteOff(int program, int note, int trackID = 0);
    void NoteOffDelay(int program, int note, int trackID = 0, f32 trackDelay = 0);

    //void DebugPlayFirstToneSample(int program, int tone);

    void ClearAllVoices(bool forceKill = true);
    void ClearAllTrackVoices(int trackID, bool forceKill = false);

    void SetSoundbank(std::unique_ptr<AliveAudioSoundbank> soundbank);


    std::recursive_mutex voiceListMutex;
    u64 currentSampleIndex = 0;

    virtual void Play(u8* stream, u32 len) override;

    // Can be changed from outside class
    AudioInterpolation Interpolation = AudioInterpolation_hermite;
    bool mAntiAliasFilteringEnabled = false;
    bool ForceReverb = false;
    f32 ReverbMix = 0.5f;
    bool DebugDisableVoiceResampling = false;

private:
    std::unique_ptr<AliveAudioSoundbank> m_Soundbank;

    std::vector<AliveAudioVoice *> m_Voices;
    std::vector<f32> m_DryChannelBuffer;
    std::vector<f32> m_ReverbChannelBuffer;

    stk::FreeVerb m_Reverb;

    void CleanVoices();
    void AliveRenderAudio(f32* AudioStream, int StreamLength);
};

///////////////
