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

class ISound;

class SoundCache
{
public:
    bool Expired() const;
    void Delete();
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
    void CacheSoundEffects();
private:
    void PlaySoundScript(const char* soundName);
    std::unique_ptr<ISound> PlaySound(const char* soundName, const char* explicitSoundBankName, bool useMusicRecord, bool useSfxRecord);
    void Preload();
    void SoundBrowserUi();
    std::unique_ptr<ISound> PlayThemeEntry(const char* entryName);
    void EnsureAmbiance();
private: // IAudioPlayer
    virtual bool Play(f32* stream, u32 len) override;
private:
    IAudioController& mAudioController;
    ResourceLocator& mLocator;
    SoundCache mCache;

    const MusicTheme* mActiveTheme = nullptr;
    ActiveMusicThemeEntry mActiveThemeEntry;

    std::mutex mSoundPlayersMutex;
    std::unique_ptr<ISound> mAmbiance;
    std::unique_ptr<ISound> mMusicTrack;

    std::vector<std::unique_ptr<ISound>> mSoundPlayers;

    InstanceBinder<class Sound> mScriptInstance;
};
