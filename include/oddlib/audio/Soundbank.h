#pragma once

#include "Sample.h"
#include <memory>

// Extended ADSR
struct VolumeEnvelope
{
    double AttackTime;
    double DecayTime;
    double SustainLevel;
    double LinearReleaseTime;
    bool ExpRelease;
};

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
    AliveAudioSoundbank(std::string lvlPath, std::string vabName, AliveAudio& aliveAudio);
    AliveAudioSoundbank(Oddlib::LvlArchive& lvlArchive, std::string vabName, AliveAudio& aliveAudio);

    void InitFromVab(Vab& vab, AliveAudio& aliveAudio);
    std::vector<std::unique_ptr<AliveAudioSample>> m_Samples;
    std::vector<std::unique_ptr<AliveAudioProgram>> m_Programs;
};
