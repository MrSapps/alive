#pragma once

#include <memory>
#include <string>
#include "asyncqueue.hpp"

class ResourceLocator;
class OSBaseFileSystem;
class ISound;
class SoundCache;

class BaseSoundCacheJob
{
public:
    BaseSoundCacheJob(SoundCache& soundCache, ResourceLocator& locator) : mSoundCache(soundCache), mLocator(locator) {  }
    virtual ~BaseSoundCacheJob() {}
    virtual void Execute(std::atomic<bool>& quitFlag) = 0;
protected:
    SoundCache& mSoundCache;
    ResourceLocator& mLocator;
};
using UP_BaseSoundCacheJob = std::unique_ptr<BaseSoundCacheJob>;

class SoundAddToCacheJob : public BaseSoundCacheJob
{
public:
    SoundAddToCacheJob(SoundCache& soundCache, ResourceLocator& locator, const std::string& name)
        : BaseSoundCacheJob(soundCache, locator), mName(name)  {  }
    
    virtual void Execute(std::atomic<bool>& quitFlag) override;

private:
    std::string mName;
};

class CacheAllSoundEffectsJob :  public BaseSoundCacheJob
{
public:
    CacheAllSoundEffectsJob(SoundCache& soundCache, ResourceLocator& locator)
        : BaseSoundCacheJob(soundCache, locator) { }

    virtual void Execute(std::atomic<bool>& quitFlag) override;
};

// Thread safe
class SoundCache
{
public:
    explicit SoundCache(OSBaseFileSystem& fs);
    ~SoundCache();
    void Sync();
    bool ExistsInMemoryCache(const std::string& name) const;
    std::unique_ptr<ISound> GetCached(const std::string& name);
    bool IsBusy() const;
    void Cancel();
    void CacheSound(ResourceLocator& locator, const std::string& name);
    void CacheAllSoundEffects(ResourceLocator& locator);
private:
    void CacheAllSoundEffectsImp(ResourceLocator& locator, std::atomic<bool>& quitFlag);

    void DeleteAll();
    void CacheSoundImpl(ResourceLocator& locator, const std::string& name, std::atomic<bool>& quitFlag);

    void AddToMemoryAndDiskCache(std::unique_ptr<ISound> sound, std::atomic<bool>& quitFlag);
    bool AddToMemoryCacheFromDiskCache(const std::string& name);
    void AsyncQueueWorkerFunction(UP_BaseSoundCacheJob item, std::atomic<bool>& quitFlag);
    void DeleteFromDiskCache(const std::string& filter);


    OSBaseFileSystem& mFs;
    std::map<std::string, std::shared_ptr<std::vector<u8>>> mSoundDataCache;
    mutable std::recursive_mutex mCacheMutex;
public:
    void RemoveFromMemoryCache(const std::string& name);
    ASyncQueue<UP_BaseSoundCacheJob> mLoaderQueue;
    std::atomic<bool> mSyncDone = false;

    friend class SoundAddToCacheJob;
    friend class CacheAllSoundEffectsJob;
};
