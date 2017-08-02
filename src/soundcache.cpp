#include "soundcache.hpp"
#include "logger.hpp"
#include "resourcemapper.hpp"
#include "audioconverter.hpp"
#include "alive_version.h"

class WavSound : public ISound
{
public:
    WavSound(const std::string& name, std::shared_ptr<std::vector<u8>>& data)
        : mName(name), mData(data)
    {
        memcpy(&mHeader.mData, data->data(), sizeof(mHeader.mData));
        mOffsetInBytes = sizeof(mHeader.mData);
    }

    virtual void Load() override { }
    virtual void DebugUi() override {}

    virtual void Play(f32* stream, u32 len) override
    {
        size_t kLenInBytes = len * sizeof(f32);

        // Handle the case where the audio call back wants N data but we only have N-X left
        if (mOffsetInBytes + kLenInBytes > mData->size())
        {
            kLenInBytes = mData->size() - mOffsetInBytes;
        }

        const f32* src = reinterpret_cast<const f32*>(mData->data() + mOffsetInBytes);
        for (auto i = 0u; i < kLenInBytes / sizeof(f32); i++)
        {
            stream[i] += src[i];
        }
        mOffsetInBytes += kLenInBytes;
    }

    virtual bool AtEnd() const override
    {
        return mOffsetInBytes >= mData->size();
    }

    virtual void Restart() override
    {
        mOffsetInBytes = sizeof(mHeader.mData);
    }

    virtual void Update() override { }
    virtual const std::string& Name() const override { return mName; }

    virtual void Stop() override
    {
        mOffsetInBytes = mData->size();
    }

private:
    size_t mOffsetInBytes = 0;
    std::string mName;
    std::shared_ptr<std::vector<u8>> mData;
    WavHeader mHeader;
};

SoundCache::SoundCache(OSBaseFileSystem& fs)
    : mFs(fs),
    mLoaderQueue([this](SoundAddToCacheJob item, std::atomic<bool>& quitFlag)
{
    AsyncQueueWorkerFunction(std::move(item), quitFlag);
})
{
    mLoaderQueue.Start();
}

SoundCache::~SoundCache()
{
    TRACE_ENTRYEXIT;
    mLoaderQueue.Stop();
}

void SoundCache::Sync()
{
    TRACE_ENTRYEXIT;

    std::lock_guard<std::mutex> lock(mCacheMutex);

    mSoundDataCache.clear();

    bool ok = false;
    std::string versionFile = "{CacheDir}/CacheVersion.txt";
    if (mFs.FileExists(versionFile))
    {
        if (mFs.Open(versionFile)->LoadAllToString() == ALIVE_VERSION)
        {
            ok = true;
        }
    }

    if (!ok)
    {
        DeleteAll();
    }

    DeleteFromDiskCache("*.tmp");
}

void SoundCache::DeleteAll()
{
    TRACE_ENTRYEXIT;

    std::lock_guard<std::mutex> lock(mCacheMutex);

    DeleteFromDiskCache("*.wav");

    // Remove from memory
    mSoundDataCache.clear();

    // Update cache version marker to current
    const std::string fileName = mFs.ExpandPath("{CacheDir}/CacheVersion.txt");
    Oddlib::FileStream versionFile(fileName, Oddlib::IStream::ReadMode::ReadWrite);
    versionFile.Write(std::string(ALIVE_VERSION));
}

void SoundCache::DeleteFromDiskCache(const std::string& filter)
{
    std::lock_guard<std::mutex> lock(mCacheMutex);

    // Delete files matching filter from {CacheDir}/disk
    std::string dirName = mFs.ExpandPath("{CacheDir}");
    const auto wavFiles = mFs.EnumerateFiles(dirName, filter.c_str());
    for (const auto& wavFile : wavFiles)
    {
        const std::string fullName = dirName + "/" + wavFile;
        LOG_INFO("Deleting: " << fullName);
        mFs.DeleteFile(fullName);
    }
}

bool SoundCache::ExistsInMemoryCache(const std::string& name) const
{
    std::lock_guard<std::mutex> lock(mCacheMutex);
    return mSoundDataCache.find(name) != std::end(mSoundDataCache);
}

std::unique_ptr<ISound> SoundCache::GetCached(const std::string& name)
{
    std::lock_guard<std::mutex> lock(mCacheMutex);
    auto it = mSoundDataCache.find(name);
    if (it != std::end(mSoundDataCache))
    {
        return std::make_unique<WavSound>(name, it->second);
    }
    return nullptr;
}

void SoundCache::AddToMemoryAndDiskCacheASync(std::unique_ptr<ISound> sound, std::atomic<bool>& quitFlag)
{
    const std::string fileName = mFs.ExpandPath("{CacheDir}/" + sound->Name() + ".wav");

    // TODO: mod files that are already wav shouldn't be converted - but could still be copied to the cache

    sound->Load();

    if (quitFlag)
    {
        return;
    }

    // TODO: Write to a .tmp file and rename when completed

    AudioConverter::Convert<WavEncoder>(*sound, fileName.c_str(), quitFlag);

    //mFs.Rename(fileName.c_str(), fileName.c_str());

    auto stream = mFs.Open(fileName);
    auto data = std::make_shared<std::vector<u8>>(Oddlib::IStream::ReadAll(*stream));;

    if (quitFlag)
    {
        return;
    }

    std::lock_guard<std::mutex> lock(mCacheMutex);
    mSoundDataCache[sound->Name()] = data;
}

void SoundCache::AsyncQueueWorkerFunction(SoundAddToCacheJob item, std::atomic<bool>& quitFlag)
{
    if (quitFlag)
    {
        return;
    }

    CacheSoundImpl(item.mLocator, item.mName, quitFlag);
}

void SoundCache::CacheSound(ResourceLocator& locator, const std::string& name)
{
    mLoaderQueue.Add(SoundAddToCacheJob(locator, name));
}

void SoundCache::CacheSoundImpl(ResourceLocator& locator, const std::string& name, std::atomic<bool>& quitFlag)
{
    if (quitFlag || ExistsInMemoryCache(name))
    {
        // Already in memory
        return;
    }

    if (quitFlag || AddToMemoryCacheFromDiskCache(name))
    {
        // Already on disk and now added to in memory cache
        return;
    }

    std::unique_ptr<ISound> pSound = locator.LocateSound(name.c_str(), nullptr, true, true);
    if (!quitFlag && pSound)
    {
        // Write into disk cache and then load from disk cache into memory cache
        AddToMemoryAndDiskCacheASync(std::move(pSound), quitFlag);
    }
}

bool SoundCache::AddToMemoryCacheFromDiskCache(const std::string& name)
{
    std::string fileName = mFs.ExpandPath("{CacheDir}/" + name + ".wav");
    if (mFs.FileExists(fileName))
    {
        auto stream = mFs.Open(fileName);
        auto data = std::make_shared<std::vector<u8>>(Oddlib::IStream::ReadAll(*stream));
        std::lock_guard<std::mutex> lock(mCacheMutex);
        mSoundDataCache[name] = data;
        return true;
    }
    return false;
}

void SoundCache::RemoveFromMemoryCache(const std::string& name)
{
    std::lock_guard<std::mutex> lock(mCacheMutex);
    auto it = mSoundDataCache.find(name);
    if (it != std::end(mSoundDataCache))
    {
        mSoundDataCache.erase(it);
    }
}
