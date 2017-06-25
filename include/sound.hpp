#pragma once

#include <memory>
#include <vector>
#include <string>
#include "oddlib/audio/AliveAudio.h"

class GameData;
class IAudioController;
class FileSystem;
class IFileSystem;
class ResourceLocator;

class Sound
{
public:
    static void RegisterScriptBindings();
    Sound(const Sound&) = delete;
    Sound& operator = (const Sound&) = delete;
    Sound(IAudioController& audioController, ResourceLocator& locator);
    ~Sound();
    void Update();
    void Render();
    void DebugUi();
private:
    void BarLoop();
    void PlaySoundEffect(const char* effectName);
private:
    void AudioSettingsUi();
    void MusicBrowserUi();
    void SoundEffectBrowserUi();
    bool mMusicBrowser = false;
    bool mSoundEffectBrowser = false;
private:
    IAudioController& mAudioController;
    ResourceLocator& mLocator;

    std::unique_ptr<class SequencePlayer> mSeqPlayer;
    AliveAudio mAliveAudio;

    // TODO: Should be removed and pre-load all sfx instead, also this pays no attention to the required sound bank
    std::map<std::string, std::unique_ptr<class ISoundEffect>> mSfxCache;

    InstanceBinder<class Sound> mScriptInstance;
};
