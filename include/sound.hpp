#pragma once

#include <memory>
#include <vector>
#include <string>
#include "oddlib/audio/AliveAudio.h"

class GameData;
class IAudioController;
class FileSystem;
class IFileSystem;
struct GuiContext;
class ResourceLocator;

class Sound
{
public:
    Sound(const Sound&) = delete;
    Sound& operator = (const Sound&) = delete;
    Sound(IAudioController& audioController, ResourceLocator& locator);
    ~Sound();
    void Update();
    void Render(GuiContext *gui, int w, int h);
private:
    void BarLoop();
private:
    void AudioSettingsUi(GuiContext* gui);
    void MusicBrowserUi(GuiContext* gui);
    void SoundEffectBrowserUi(GuiContext* gui);
    bool mMusicBrowser = false;
    bool mSoundEffectBrowser = false;

    bool mAePc = false;
    std::vector<std::string> mFilteredSoundEffectResources;
private:
    IAudioController& mAudioController;
    ResourceLocator& mLocator;

    std::unique_ptr<class SequencePlayer> mSeqPlayer;
    AliveAudio mAliveAudio;
};
