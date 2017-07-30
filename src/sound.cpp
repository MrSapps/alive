#include "sound.hpp"
#include "oddlib/audio/SequencePlayer.h"
#include "core/audiobuffer.hpp"
#include "logger.hpp"
#include "resourcemapper.hpp"
#include "audioconverter.hpp"
#include "alive_version.h"

SoundCache::SoundCache(OSBaseFileSystem& fs)
    : mFs(fs)
{

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
}

void SoundCache::DeleteAll()
{
    TRACE_ENTRYEXIT;
    
    std::lock_guard<std::mutex> lock(mCacheMutex);

    // Delete *.wav from {CacheDir}/disk
    std::string dirName = mFs.ExpandPath("{CacheDir}");
    const auto wavFiles = mFs.EnumerateFiles(dirName, "*.wav");
    for (const auto& wavFile : wavFiles)
    {
        mFs.DeleteFile(dirName + "/" + wavFile);
    }

    // Remove from memory
    mSoundDataCache.clear();

    // Update cache version marker to current
    const std::string fileName = mFs.ExpandPath("{CacheDir}/CacheVersion.txt");
    Oddlib::FileStream versionFile(fileName, Oddlib::IStream::ReadMode::ReadWrite);
    versionFile.Write(std::string(ALIVE_VERSION));
}

bool SoundCache::ExistsInMemoryCache(const std::string& name) const
{
    std::lock_guard<std::mutex> lock(mCacheMutex);
    return mSoundDataCache.find(name) != std::end(mSoundDataCache);
}

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
        for (auto i = 0u; i < kLenInBytes/sizeof(f32); i++)
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

std::unique_ptr<ISound> SoundCache::GetCached(const std::string& name)
{
    auto it = mSoundDataCache.find(name);
    if (it != std::end(mSoundDataCache))
    {
        return std::make_unique<WavSound>(name, it->second);
    }
    return nullptr;
}

void SoundCache::AddToMemoryAndDiskCacheASync(ISound& sound)
{
    const std::string fileName = mFs.ExpandPath("{CacheDir}/" + sound.Name() + ".wav");

    // TODO: mod files that are already wav shouldn't be converted
    sound.Load();
    AudioConverter::Convert<WavEncoder>(sound, fileName.c_str());

    auto stream = mFs.Open(fileName);
    mSoundDataCache[sound.Name()] = std::make_shared<std::vector<u8>>(Oddlib::IStream::ReadAll(*stream));
}

bool SoundCache::AddToMemoryCacheFromDiskCache(const std::string& name)
{
    std::string fileName = mFs.ExpandPath("{CacheDir}/" + name + ".wav");
    if (mFs.FileExists(fileName))
    {
        auto stream = mFs.Open(fileName);
        mSoundDataCache[name] = std::make_shared<std::vector<u8>>(Oddlib::IStream::ReadAll(*stream));
        return true;
    }
    return false;
}

void SoundCache::RemoveFromMemoryCache(const std::string& name)
{
    auto it = mSoundDataCache.find(name);
    if (it != std::end(mSoundDataCache))
    {
        mSoundDataCache.erase(it);
    }
}

// ====================================================================

void Sound::PlaySoundEffect(const char* soundName)
{
    auto ret = PlaySound(soundName, nullptr, true, true, true);
    if (ret)
    {
        std::lock_guard<std::mutex> lock(mSoundPlayersMutex);
        mSoundPlayers.push_back(std::move(ret));
    }
}

bool Sound::IsLoading() const
{
    return mState != eSoundStates::eIdle;
}

std::unique_ptr<ISound> Sound::PlaySound(const char* soundName, const char* explicitSoundBankName, bool useMusicRec, bool useSfxRec, bool useCache)
{
    if (useCache)
    {
        std::unique_ptr<ISound> pSound = mCache.GetCached(soundName);
        if (pSound)
        {
            LOG_INFO("Play sound cached: " << soundName);
        }
        else
        {
            LOG_ERROR("Cached sound not found: " << soundName);
        }
        return pSound;
    }
    else
    {
        std::unique_ptr<ISound> pSound = mLocator.LocateSound(soundName, explicitSoundBankName, useMusicRec, useSfxRec);
        if (pSound)
        {
            LOG_INFO("Play sound: " << soundName);
            pSound->Load();
        }
        else
        {
            LOG_ERROR("Sound: " << soundName << " not found");
        }
        return pSound;
    }
}

void Sound::SetMusicTheme(const char* themeName)
{
    //CacheMemoryResidentSounds();

    mAmbiance = nullptr;
    mMusicTrack = nullptr;

    // This is just an in-memory non blocking look up
    mThemeToLoad = mLocator.LocateSoundTheme(themeName);
    if (mThemeToLoad)
    {
        mState = eSoundStates::eLoadingSoundTheme;
        SetLoadingSoundThemeState(eLoadingSoundThemeStates::eUnloadingActiveTheme);
    }
    else
    {
        LOG_ERROR("Music theme " << themeName << " was not found");
    }
}

up_future_void Sound::CacheMemoryResidentSounds()
{
    return std::make_unique<future_void>(std::async(std::launch::async, [&]() 
    {
        TRACE_ENTRYEXIT;

        mState = eSoundStates::eLoadingSoundEffects;

        // initial one time sync
        mCache.Sync();

        const std::vector<SoundResource>& resources = mLocator.GetSoundResources();
        for (const SoundResource& resource : resources)
        {
            if (resource.mIsCacheResident)
            {
                CacheSound(resource.mResourceName);
            }
        }
    }));
}

void Sound::CacheActiveTheme(bool add)
{
    for (auto& entry : mActiveTheme->mEntries)
    {
        for (auto& e : entry.second)
        {
            if (add)
            {
                CacheSound(e.mMusicName);
            }
            else
            {
                mCache.RemoveFromMemoryCache(e.mMusicName);
            }
        }
    }
}

void Sound::CacheSound(const std::string& name)
{
    if (mCache.ExistsInMemoryCache(name))
    {
        // Already in memory
        return;
    }

    if (mCache.AddToMemoryCacheFromDiskCache(name))
    {
        // Already on disk and now added to in memory cache
        return;
    }

    std::unique_ptr<ISound> pSound = mLocator.LocateSound(name.c_str(), nullptr, true, true);
    if (pSound)
    {
        // Write into disk cache and then load from disk cache into memory cache
        mCache.AddToMemoryAndDiskCacheASync(*pSound);
    }
}

void Sound::HandleMusicEvent(const char* eventName)
{
    // TODO: Need quarter beat transition in some cases
    EnsureAmbiance();

    if (strcmp(eventName, "AMBIANCE") == 0)
    {
        mMusicTrack = nullptr;
        return;
    }

    auto ret = PlayThemeEntry(eventName);
    if (ret)
    {
        mMusicTrack = std::move(ret);
    }
}

std::unique_ptr<ISound> Sound::PlayThemeEntry(const char* entryName)
{
    if (mActiveTheme)
    {
        mActiveThemeEntry.SetMusicThemeEntry(mActiveTheme->FindEntry(entryName));

        const MusicThemeEntry* entry = mActiveThemeEntry.Entry();
        if (entry)
        {
            return PlaySound(entry->mMusicName.c_str(), nullptr, true, true, true);
        }
    }
    return nullptr;
}

void Sound::EnsureAmbiance()
{
    std::lock_guard<std::mutex> lock(mSoundPlayersMutex);
    if (!mAmbiance)
    {
        mAmbiance = PlayThemeEntry("AMBIANCE");
    }
}

// Audio thread context
bool Sound::Play(f32* stream, u32 len)
{
    std::lock_guard<std::mutex> lock(mSoundPlayersMutex);

    if (mAmbiance)
    {
        mAmbiance->Play(stream, len);
    }

    if (mMusicTrack)
    {
        mMusicTrack->Play(stream, len);
    }

    for (auto& player : mSoundPlayers)
    {
        player->Play(stream, len);
    }
    return false;
}

Sound::Sound(IAudioController& audioController, ResourceLocator& locator, OSBaseFileSystem& fs)
    : mAudioController(audioController), mLocator(locator), mCache(fs)
{
    mAudioController.AddPlayer(this);

    Debugging().AddSection([&]()
    {
        SoundBrowserUi();
    });
}

Sound::~Sound()
{
    // Ensure the audio call back stops at this point else will be
    // calling back into freed objects
    SDL_AudioQuit();

    mAudioController.RemovePlayer(this);
}

void Sound::HandleLoadingSoundTheme()
{
    switch (mLoadingSoundThemeState)
    {
    case Sound::eLoadingSoundThemeStates::eIdle:
        SetState(eSoundStates::eIdle);
        break;

    case Sound::eLoadingSoundThemeStates::eUnloadingActiveTheme:
        if (mActiveTheme)
        {
            CacheActiveTheme(false);
        }
        mActiveTheme = mThemeToLoad;
        mThemeToLoad = nullptr;
        SetLoadingSoundThemeState(eLoadingSoundThemeStates::eLoadingActiveTheme);
        break;

    case eLoadingSoundThemeStates::eLoadingActiveTheme:
        if (mActiveTheme)
        {
            CacheActiveTheme(true);
        }
        SetLoadingSoundThemeState(eLoadingSoundThemeStates::eIdle);
        break;
    }
}

void Sound::SetState(Sound::eSoundStates state)
{
    if (mState != state)
    {
        mState = state;
        SetLoadingSoundThemeState(eLoadingSoundThemeStates::eIdle);
    }
}

void Sound::SetLoadingSoundThemeState(Sound::eLoadingSoundThemeStates state)
{
    mLoadingSoundThemeState = state;
}

void Sound::Update()
{
    switch (mState)
    {
    case eSoundStates::eIdle:
        break;

    case eSoundStates::eLoadingSoundEffects:
        break;

    case eSoundStates::eLoadingSoundTheme:
        HandleLoadingSoundTheme();
        break;
    }

    std::lock_guard<std::mutex> lock(mSoundPlayersMutex);
    for (auto it = mSoundPlayers.begin(); it != mSoundPlayers.end();)
    {
        if ((*it)->AtEnd())
        {
            it = mSoundPlayers.erase(it);
        }
        else
        {
            it++;
        }
    }

    if (mAmbiance)
    {
        mAmbiance->Update();
        if (mAmbiance->AtEnd())
        {
            mAmbiance->Restart();
        }
    }

    if (mMusicTrack)
    {
        mMusicTrack->Update();
        if (mMusicTrack->AtEnd())
        {
            if (mActiveThemeEntry.ToNextEntry())
            {
                mMusicTrack = PlaySound(mActiveThemeEntry.Entry()->mMusicName.c_str(), nullptr, true, true, true);
            }
            else
            {
                mMusicTrack = nullptr;
            }
        }
    }

    for (auto& player : mSoundPlayers)
    {
        player->Update();
    }
}

static const char* kMusicEvents[] =
{
    "AMBIANCE",
    "BASE_LINE",
    "CRITTER_ATTACK",
    "CRITTER_PATROL",
    "SLIG_ATTACK",
    "SLIG_PATROL",
    "SLIG_POSSESSED",
};


void Sound::SoundBrowserUi()
{
    {
        std::lock_guard<std::mutex> lock(mSoundPlayersMutex);
        if (!mSoundPlayers.empty())
        {
            mSoundPlayers[0]->DebugUi();
        }

        if (ImGui::CollapsingHeader("Active SEQs"))
        {
            if (mAmbiance)
            {
                ImGui::Text("Ambiance: %s", mAmbiance->Name().c_str());
            }

            if (mMusicTrack)
            {
                ImGui::Text("Music: %s", mMusicTrack->Name().c_str());
            }

            int i = 0;
            for (auto& player : mSoundPlayers)
            {
                i++;
                if (ImGui::Button((std::to_string(i) + player->Name()).c_str()))
                {
                    if (player.get() == mSoundPlayers[i].get())
                    {
                        player->Stop();
                    }
                }
            }

            if (!mAmbiance && !mMusicTrack && mSoundPlayers.empty())
            {
                ImGui::TextUnformatted("(none)");
            }
        }
    }

    if (ImGui::CollapsingHeader("Sound list"))
    {
        static const SoundResource* selected = nullptr;

        // left
        ImGui::BeginChild("left pane", ImVec2(200, 200), true);
        {
            for (const SoundResource& soundInfo : mLocator.GetSoundResources())
            {
                if (ImGui::Selectable(soundInfo.mResourceName.c_str()))
                {
                    selected = &soundInfo;
                }
            }
            ImGui::EndChild();
        }
        ImGui::SameLine();

        // right
        ImGui::BeginGroup();
        {
            ImGui::BeginChild("item view", ImVec2(0, 200));
            {
                if (selected)
                {
                    ImGui::TextWrapped("Resource name: %s", selected->mResourceName.c_str());
                    ImGui::Separator();
                    ImGui::TextWrapped("Comment: %s", selected->mComment.empty() ? "(none)" : selected->mComment.c_str());
                    ImGui::Separator();
                    ImGui::TextWrapped("Is memory resident: %s", selected->mIsCacheResident ? "true" : "false");
                    ImGui::TextWrapped("Is cached: %s", mCache.ExistsInMemoryCache(selected->mResourceName) ? "true" : "false");

                    static bool bUseCache = false;
                    ImGui::Checkbox("Use cache", &bUseCache);

                    const bool bHasMusic = !selected->mMusic.mSoundBanks.empty();
                    const bool bHasSample = !selected->mSoundEffect.mSoundBanks.empty();
                    if (bHasMusic)
                    {
                        if (ImGui::CollapsingHeader("SEQs"))
                        {
                            for (const std::string& sb : selected->mMusic.mSoundBanks)
                            {
                                if (ImGui::Selectable(sb.c_str()))
                                {
                                    auto player = PlaySound(selected->mResourceName.c_str(), sb.c_str(), true, false, bUseCache);
                                    if (player)
                                    {
                                        std::lock_guard<std::mutex> lock(mSoundPlayersMutex);
                                        mSoundPlayers.push_back(std::move(player));
                                    }
                                }
                            }
                        }
                    }

                    if (bHasSample)
                    {
                        if (ImGui::CollapsingHeader("Samples"))
                        {
                            for (const SoundEffectResourceLocation& sbLoc : selected->mSoundEffect.mSoundBanks)
                            {
                                for (const std::string& sb : sbLoc.mSoundBanks)
                                {
                                    if (ImGui::Selectable(sb.c_str()))
                                    {
                                        auto player = PlaySound(selected->mResourceName.c_str(), sb.c_str(), false, true, bUseCache);
                                        if (player)
                                        {
                                            std::lock_guard<std::mutex> lock(mSoundPlayersMutex);
                                            mSoundPlayers.push_back(std::move(player));
                                        }
                                    }
                                }
                            }
                        }
                    }

                    if (ImGui::Button("Play (cached/scripted)"))
                    {
                        PlaySoundEffect(selected->mResourceName.c_str());
                    }
                }
                else
                {
                    ImGui::TextWrapped("Click an item to display its info");
                }
            }
            ImGui::EndChild();
        }
        ImGui::EndGroup();
    }

    if (ImGui::CollapsingHeader("Sound themes"))
    {
        for (const MusicTheme& theme : mLocator.mResMapper.mSoundResources.mThemes)
        {
            if (ImGui::RadioButton(theme.mName.c_str(), mActiveTheme && theme.mName == mActiveTheme->mName))
            {
                SetMusicTheme(theme.mName.c_str());
                HandleMusicEvent("BASE_LINE");
            }
        }

        for (const char* eventName : kMusicEvents)
        {
            if (ImGui::Button(eventName))
            {
                HandleMusicEvent(eventName);
            }
        }
    }
}
