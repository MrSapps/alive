#pragma once

#include "core/system.hpp"

class Sound;

class SoundSystem : public System
{
public:
    DECLARE_SYSTEM(SoundSystem);
    explicit SoundSystem(Sound& sound);
    void PlaySoundEffect(const char* name);
private:
    Sound& mSound;
};
