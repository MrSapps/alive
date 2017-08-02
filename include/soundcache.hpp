#pragma once

#include <memory>
#include <string>
#include "asyncqueue.hpp"

class ResourceLocator;
class OSBaseFileSystem;
class ISound;

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

// Thread safe
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
    void CacheSoundImpl(ResourceLocator& locator, const std::string& name, std::atomic<bool>& quitFlag);

    void AddToMemoryAndDiskCacheASync(std::unique_ptr<ISound> sound, std::atomic<bool>& quitFlag);
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
