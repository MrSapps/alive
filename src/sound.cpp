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
    // TODO: Delete *.wav from {CacheDir}
    
    mSoundDataCache.clear();

    const std::string fileName = mFs.ExpandPath("{CacheDir}/CacheVersion.txt");
    Oddlib::FileStream versionFile(fileName, Oddlib::IStream::ReadMode::ReadWrite);
    versionFile.Write(std::string(ALIVE_VERSION));
}

bool SoundCache::ExistsInMemoryCache(const std::string& name) const
{
    return mSoundDataCache.find(name) != std::end(mSoundDataCache);
}

class WavSound : public ISound
{
public:
    WavSound(const std::string& name, std::shared_ptr<std::vector<u8>>& data)
        : mName(name), mData(data)
    {
        //mHeader.Read(data);

    }

    virtual void Load() override { }
    virtual void DebugUi() override {}

    virtual void Play(f32* /*stream*/, u32 len) override
    {
        mOffset += len;
    }

    virtual bool AtEnd() const
    {
        return mOffset == mData->size();
    }

    virtual void Restart()
    {
        mOffset = 0;
    }

    virtual void Update() { }
    virtual const std::string& Name() const { return mName; }

private:
    size_t mOffset = 0;
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

void SoundCache::AddToMemoryAndDiskCache(ISound& sound)
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

void Sound::PlaySoundScript(const char* soundName)
{
    auto ret = PlaySound(soundName, nullptr, true, true, true);
    if (ret)
    {
        std::lock_guard<std::mutex> lock(mSoundPlayersMutex);
        mSoundPlayers.push_back(std::move(ret));
    }
}
std::unique_ptr<ISound> Sound::PlaySound(const char* soundName, const char* explicitSoundBankName, bool useMusicRec, bool useSfxRec, bool useCache)
{
    if (useCache)
    {
        LOG_INFO("Play sound cached: " << soundName);
        return mCache.GetCached(soundName);
    }

    std::unique_ptr<ISound> pSound = mLocator.LocateSound(soundName, explicitSoundBankName, useMusicRec, useSfxRec);
    if (pSound)
    {
        LOG_INFO("Play sound: " << soundName);
        pSound->Load();
        return pSound;
    }
    else
    {
        LOG_ERROR("Sound: " << soundName << " not found");
    }
    return nullptr;
}

void Sound::SetTheme(const char* themeName)
{
    mAmbiance = nullptr;
    mMusicTrack = nullptr;
    const MusicTheme* newTheme = mLocator.LocateSoundTheme(themeName);
    
    // Remove current musics from in memory cache
    if (mActiveTheme)
    {
        CacheActiveTheme(false);
    }

    mActiveTheme = newTheme;

    // Add new musics to in memory cache
    if (mActiveTheme)
    {
        CacheActiveTheme(true);
    }
    else
    {
        LOG_ERROR("Music theme " << themeName << " was not found");
    }
}

void Sound::CacheSoundEffects()
{
    TRACE_ENTRYEXIT;

    // initial one time sync
    mCache.Sync();

    const std::vector<SoundResource>& resources = mLocator.SoundResources();
    for (const SoundResource& resource : resources)
    {
        if (resource.mIsSoundEffect)
        {
            CacheSound(resource.mResourceName);
        }
    }
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
        mCache.AddToMemoryAndDiskCache(*pSound);
    }
}

void Sound::HandleEvent(const char* eventName)
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

/*static*/ void Sound::RegisterScriptBindings()
{
    Sqrat::Class<Sound, Sqrat::NoConstructor<Sound>> c(Sqrat::DefaultVM::Get(), "Sound");
    c.Func("PlaySoundEffect", &Sound::PlaySoundScript);
    c.Func("SetTheme", &Sound::SetTheme);
    Sqrat::RootTable().Bind("Sound", c);
}

Sound::Sound(IAudioController& audioController, ResourceLocator& locator, OSBaseFileSystem& fs)
    : mAudioController(audioController), mLocator(locator), mCache(fs), mScriptInstance("gSound", this)
{
    mAudioController.AddPlayer(this);
}

Sound::~Sound()
{
    // Ensure the audio call back stops at this point else will be
    // calling back into freed objects
    SDL_AudioQuit();

    mAudioController.RemovePlayer(this);
}

void Sound::Update()
{
    if (Debugging().mBrowserUi.soundBrowserOpen)
    {
        {
            std::lock_guard<std::mutex> lock(mSoundPlayersMutex);
            if (!mSoundPlayers.empty())
            {
                mSoundPlayers[0]->DebugUi();
            }

            if (ImGui::Begin("Active SEQs"))
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
                        // TODO: Kill whatever this SEQ is
                    }
                }
            }
            ImGui::End();
        }

        SoundBrowserUi();
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
    ImGui::Begin("Sound list");

    static const SoundResource* selected = nullptr;

    // left
    ImGui::BeginChild("left pane", ImVec2(200, 0), true);
    {
        for (const SoundResource& soundInfo : mLocator.mResMapper.mSoundResources.mSounds)
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
        ImGui::BeginChild("item view");
        {
            if (selected)
            {
                ImGui::TextWrapped("Resource name: %s", selected->mResourceName.c_str());
                ImGui::Separator();
                ImGui::TextWrapped("Comment: %s", selected->mComment.empty() ? "(none)" : selected->mComment.c_str());
                ImGui::Separator();
                ImGui::TextWrapped("Is sound effect: %s", selected->mIsSoundEffect ? "true" : "false");

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
                    PlaySoundScript(selected->mResourceName.c_str());
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

    ImGui::End();

    ImGui::Begin("Sound themes");

    for (const MusicTheme& theme : mLocator.mResMapper.mSoundResources.mThemes)
    {
        if (ImGui::RadioButton(theme.mName.c_str(), mActiveTheme && theme.mName == mActiveTheme->mName))
        {
            SetTheme(theme.mName.c_str());
            HandleEvent("BASE_LINE");
        }
    }

    for (const char* eventName : kMusicEvents)
    {
        if (ImGui::Button(eventName))
        {
            HandleEvent(eventName);
        }
    }

    ImGui::End();

}

void Sound::Render(int /*w*/, int /*h*/)
{

}
