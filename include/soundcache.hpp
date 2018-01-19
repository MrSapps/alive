#pragma once

#include <memory>
#include <string>
#include <map>
#include <vector>
#include <mutex>
#include "jobsystem.hpp"
#include "types.hpp"

class ResourceLocator;
class OSBaseFileSystem;
class ISound;
class SoundCache;

class BaseSoundCacheJob : public IJob
{
public:
    BaseSoundCacheJob(SoundCache& soundCache, ResourceLocator& locator) : mSoundCache(soundCache), mLocator(locator) 
    {

    }

    virtual ~BaseSoundCacheJob() {}

    virtual void OnFinished() override
    {

    }
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
    
    virtual void OnExecute(const CancelFlag& quitFlag) override;

private:
    std::string mName;
};

class CacheAllSoundEffectsJob :  public BaseSoundCacheJob
{
public:
    CacheAllSoundEffectsJob(SoundCache& soundCache, ResourceLocator& locator)
        : BaseSoundCacheJob(soundCache, locator) { }

    virtual void OnExecute(const CancelFlag& quitFlag) override;
};

// Thread safe
class SoundCache
{
public:
    SoundCache(OSBaseFileSystem& fs, JobSystem& jobSystem);
    ~SoundCache();
    void Sync();
    bool ExistsInMemoryCache(const std::string& name) const;
    std::unique_ptr<ISound> GetCached(const std::string& name);
    bool IsBusy() const;
    void Cancel();
    void CacheSound(ResourceLocator& locator, const std::string& name);
    void CacheAllSoundEffects(ResourceLocator& locator);
private:
    void CacheAllSoundEffectsImp(ResourceLocator& locator, const CancelFlag& quitFlag);

    void DeleteAll();
    void CacheSoundImpl(ResourceLocator& locator, const std::string& name, const CancelFlag& quitFlag);

    void AddToMemoryAndDiskCache(std::unique_ptr<ISound> sound, const CancelFlag& quitFlag);
    bool AddToMemoryCacheFromDiskCache(const std::string& name);
    void AsyncQueueWorkerFunction(UP_BaseSoundCacheJob item, const CancelFlag& quitFlag);
    void DeleteFromDiskCache(const std::string& filter);


    OSBaseFileSystem& mFs;
    JobSystem& mJobSystem;
    JobTracker mJobTracker;
    std::map<std::string, std::shared_ptr<std::vector<u8>>> mSoundDataCache;
    mutable std::recursive_mutex mCacheMutex;
public:
    void RemoveFromMemoryCache(const std::string& name);
    std::atomic_bool mSyncDone{ false };

    friend class SoundAddToCacheJob;
    friend class CacheAllSoundEffectsJob;
};
