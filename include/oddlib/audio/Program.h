#pragma once

#include "Tone.h"
#include <vector>
#include <memory>

class AliveAudioProgram
{
public:
    std::vector<std::unique_ptr<AliveAudioTone>> m_Tones;
};
