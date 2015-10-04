#pragma once

#include "Sample.h"
#include "Tone.h"
#include <memory>

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
