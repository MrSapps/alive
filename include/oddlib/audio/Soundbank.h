#pragma once

#include "types.hpp"
#include "Sample.h"
#include <memory>

// Extended ADSR
struct VolumeEnvelope
{
    f64 AttackTime;
    f64 DecayTime;
    f64 SustainLevel;
    f64 LinearReleaseTime;
    bool ExpRelease;
};

class AliveAudioTone
{
public:
    // volume 0-1
    f32 f_Volume;

    // panning -1 - 1
    f32 f_Pan;

    // Root Key - when played at this "key" it is playing the "raw" sample
    unsigned char mMidiRootKey;
    unsigned char c_Shift;

    // Key range
    unsigned char Min;
    unsigned char Max;

    f32 Pitch;
    bool Reverbate;

    VolumeEnvelope Env;

    bool Loop = false;

    // Not owned
    AliveAudioSample * m_Sample = nullptr;
};

class AliveAudioProgram
{
public:
    std::vector<std::unique_ptr<AliveAudioTone>> m_Tones;
};

class AliveAudio;
class AliveAudioSoundbank
{
public:
    ~AliveAudioSoundbank() = default;
    AliveAudioSoundbank(const AliveAudioSoundbank&) = delete;
    AliveAudioSoundbank& operator = (const AliveAudioSoundbank&) = delete;
    AliveAudioSoundbank(Vab& vab);
    void InitFromVab(Vab& vab);

    std::vector<std::unique_ptr<AliveAudioSample>> m_Samples;
    std::vector<std::unique_ptr<AliveAudioProgram>> m_Programs;
};
