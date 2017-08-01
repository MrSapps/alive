#pragma once

#include <memory>
#include <vector>
#include <string>
#include <map>
#include <mutex>
#include <deque>
#include <future>
#include "proxy_sqrat.hpp"
#include "core/audiobuffer.hpp"
#include "asyncqueue.hpp"

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
class OSBaseFileSystem;

namespace Oddlib
{
    class MemoryStream;
}

// Thread safe

class SoundAddToCacheJob
{
public:
    SoundAddToCacheJob(ResourceLocator& locator, const std::string& name)
        : mLocator(locator), mName(name)
    {

    }

public:
    std::reference_wrapper<ResourceLocator> mLocator;
    std::string mName;
};

class SoundCache
{
public:
    explicit SoundCache(OSBaseFileSystem& fs);
    ~SoundCache();
    void Sync();
    bool ExistsInMemoryCache(const std::string& name) const;
    std::unique_ptr<ISound> GetCached(const std::string& name);
    bool IsBusy() const { return mLoaderQueue.IsIdle() == false; }
    void CacheSound(ResourceLocator& locator, const std::string& name);
private:
    void DeleteAll();
    void CacheSoundImpl(ResourceLocator& locator, const std::string& name);

    void AddToMemoryAndDiskCacheASync(std::unique_ptr<ISound> sound);
    bool AddToMemoryCacheFromDiskCache(const std::string& name);
    void AsyncQueueWorkerFunction(SoundAddToCacheJob item, std::atomic<bool>& quitFlag);
    void DeleteFromDiskCache(const std::string& filter);


    OSBaseFileSystem& mFs;
    std::map<std::string, std::shared_ptr<std::vector<u8>>> mSoundDataCache;
    mutable std::mutex mCacheMutex;
public:
    void RemoveFromMemoryCache(const std::string& name);
    ASyncQueue<SoundAddToCacheJob> mLoaderQueue;
};

using future_void = std::future<void>;
using up_future_void = std::unique_ptr<future_void>;

class Sound : public IAudioPlayer
{
public:
    Sound(const Sound&) = delete;
    Sound& operator = (const Sound&) = delete;
    Sound(IAudioController& audioController, ResourceLocator& locator, OSBaseFileSystem& fs);
    ~Sound();

    void SetMusicTheme(const char* themeName);
    bool IsLoading() const;

    void Update();

    void HandleMusicEvent(const char* eventName);
    void PlaySoundEffect(const char* soundName);

private:
    up_future_void CacheMemoryResidentSounds();

    void CacheActiveTheme(bool add);
    std::unique_ptr<ISound> PlaySound(const char* soundName, const char* explicitSoundBankName, bool useMusicRecord, bool useSfxRecord, bool useCache);
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
    const MusicTheme* mThemeToLoad = nullptr;
    ActiveMusicThemeEntry mActiveThemeEntry;

    std::mutex mSoundPlayersMutex;

    // Thread safe
    std::unique_ptr<ISound> mAmbiance;

    // Thread safe
    std::unique_ptr<ISound> mMusicTrack;

    // Thread safe
    std::vector<std::unique_ptr<ISound>> mSoundPlayers;

    enum class eSoundStates
    {
        eLoadingSoundEffects,
        eUnloadingActiveSoundTheme,
        eLoadActiveSoundTheme,
        eLoadingActiveSoundTheme,
        eIdle
    };
    eSoundStates mState = eSoundStates::eIdle;

    void SetState(Sound::eSoundStates state);
};
