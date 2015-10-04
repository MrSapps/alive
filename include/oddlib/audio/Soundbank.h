#pragma once

#include "Sample.h"
#include <memory>


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
    ~AliveAudioSoundbank();
    AliveAudioSoundbank(const AliveAudioSoundbank&) = delete;
    AliveAudioSoundbank& operator = (const AliveAudioSoundbank&) = delete;
    AliveAudioSoundbank(std::string fileName, AliveAudio& aliveAudio);
    AliveAudioSoundbank(std::string lvlPath, std::string vabName, AliveAudio& aliveAudio);
    AliveAudioSoundbank(Oddlib::LvlArchive& lvlArchive, std::string vabName, AliveAudio& aliveAudio);

    void InitFromVab(Vab& vab, AliveAudio& aliveAudio);
    std::vector<std::unique_ptr<AliveAudioSample>> m_Samples;
    std::vector<std::unique_ptr<AliveAudioProgram>> m_Programs;
};
