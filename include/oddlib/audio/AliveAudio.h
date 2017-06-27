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

const int kAliveAudioSampleRate = 44100;

class FileSystem;

class AliveAudio
{
public:
    AliveAudio() = default;
    AliveAudio(AliveAudio&&) = delete;
    AliveAudio(const AliveAudio&) = delete;
    AliveAudio& operator = (const AliveAudio&) = delete;
    AliveAudio& operator = (AliveAudio&&) = delete;

    void NoteOn(int program, int note, char velocity, f64 trackDelay = 0, f64 pitch = 0.0f);
    void NoteOff(int program, int note);
    void NoteOffDelay(int program, int note, f32 trackDelay = 0);
    void ClearAllVoices(bool forceKill = true);
    void ClearAllTrackVoices(bool forceKill = false);

    void SetSoundbank(std::unique_ptr<AliveAudioSoundbank> soundbank);

    u64 mCurrentSampleIndex = 0;

    void Play(f32* stream, u32 len);

    u32 NumberOfActiveVoices() const { return static_cast<u32>(m_Voices.size()); }

    // Can be changed from outside class
    AudioInterpolation Interpolation = AudioInterpolation_hermite;
    bool ForceReverb = false;
    f32 ReverbMix = 0.5f;
    bool DebugDisableVoiceResampling = false;

    // TODO: Temp for sound effect debugging
    void VabBrowserUi();
private:
    std::unique_ptr<AliveAudioSoundbank> m_Soundbank;

    std::vector<AliveAudioVoice *> m_Voices;
    std::vector<f32> m_DryChannelBuffer;
    std::vector<f32> m_ReverbChannelBuffer;

    stk::FreeVerb m_Reverb;

    void CleanVoices();
    void AliveRenderAudio(f32* AudioStream, int StreamLength);
};
