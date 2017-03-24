#pragma once

#include <memory>
#include <vector>
#include <string>
#include "oddlib/audio/AliveAudio.h"
#include "proxy_sol.hpp"

class GameData;
class IAudioController;
class FileSystem;
class IFileSystem;
struct GuiContext;
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
    void Render(GuiContext *gui, int w, int h);
private:
    void BarLoop();
    void PlaySoundEffect(const char* effectName);
private:
    void AudioSettingsUi(GuiContext* gui);
    void MusicBrowserUi(GuiContext* gui);
    void SoundEffectBrowserUi(GuiContext* gui);
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
