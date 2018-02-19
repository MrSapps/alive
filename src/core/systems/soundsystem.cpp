#include "core/systems/soundsystem.hpp"
#include "sound.hpp"
#include "sound_resources.hpp"

DEFINE_SYSTEM(SoundSystem);

SoundSystem::SoundSystem(Sound& sound)
    : mSound(sound)
{

}

void SoundSystem::PlaySoundEffect(const char* name)
{
    // TODO: Probably need to return the sound id so it can be stopped etc
    mSound.PlaySoundEffect(name);
}
