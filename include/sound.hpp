#pragma once

#include <memory>
#include <vector>
#include <string>
#include <map>
#include <mutex>
#include "proxy_sqrat.hpp"
#include "core/audiobuffer.hpp"

class GameData;
class IAudioController;
class IFileSystem;
class ResourceLocator;
class MusicTheme;
class MusicThemeEntry;
class SequencePlayer;

class ActiveMusicThemeEntry
{
public:
    ActiveMusicThemeEntry() = default;

    void SetMusicThemeEntry(const std::vector<MusicThemeEntry>* theme)
    {
        mActiveTheme = theme;
        mEntryIndex = 0;
    }

    bool ToNextEntry()
    {
        if (!mActiveTheme)
        {
            return false;
        }

        if (mEntryIndex+1 < mActiveTheme->size())
        {
            // TODO: Check loop count
            mEntryIndex++;
            return true;
        }
        return false;
    }

    const MusicThemeEntry* Entry() const
    {
        if (!mActiveTheme)
        {
            return nullptr;
        }
        return &(*mActiveTheme)[mEntryIndex];
    }
private:
    u32 mEntryIndex = 0;
    const std::vector<MusicThemeEntry>* mActiveTheme = nullptr;
};

class Sound : public IAudioPlayer
{
public:
    static void RegisterScriptBindings();
    Sound(const Sound&) = delete;
    Sound& operator = (const Sound&) = delete;
    Sound(IAudioController& audioController, ResourceLocator& locator);
    ~Sound();
    void Update();
    void Render(int w, int h);
    void HandleEvent(const char* eventName);
    void SetTheme(const char* themeName);
private:
    void PlaySoundScript(const char* soundName);
    std::unique_ptr<SequencePlayer> PlaySound(const char* soundName, const char* explicitSoundBankName, bool useMusicRecord, bool useSfxRecord);
    void Preload();
    void SoundBrowserUi();
    std::unique_ptr<SequencePlayer> PlayThemeEntry(const char* entryName);
    void EnsureAmbiance();
private: // IAudioPlayer
    virtual bool Play(f32* stream, u32 len) override;
private:
    IAudioController& mAudioController;
    ResourceLocator& mLocator;

    const MusicTheme* mActiveTheme = nullptr;
    ActiveMusicThemeEntry mActiveThemeEntry;

    std::recursive_mutex mSeqPlayersMutex;
    std::unique_ptr<SequencePlayer> mAmbiance;
    std::unique_ptr<SequencePlayer> mMusicTrack;

    std::vector<std::unique_ptr<SequencePlayer>> mSeqPlayers;

    InstanceBinder<class Sound> mScriptInstance;
};
