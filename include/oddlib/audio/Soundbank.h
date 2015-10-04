#pragma once

#include "Sample.h"
#include "Program.h"
#include <memory>

class AliveAudio;
class AliveAudioSoundbank
{
public:
    ~AliveAudioSoundbank();
    AliveAudioSoundbank(std::string fileName, AliveAudio& aliveAudio);
    AliveAudioSoundbank(std::string lvlPath, std::string vabName, AliveAudio& aliveAudio);
    AliveAudioSoundbank(Oddlib::LvlArchive& lvlArchive, std::string vabName, AliveAudio& aliveAudio);

    void InitFromVab(Vab& vab, AliveAudio& aliveAudio);
    std::vector<AliveAudioSample *> m_Samples;
    std::vector<std::unique_ptr<AliveAudioProgram>> m_Programs;
};
