#pragma once

#include "data_inspector.hpp"
#include "sound_resources.hpp"
#include <set>

class TempSoundEffectResource : public SoundEffectResource
{
public:
    TempSoundEffectResource() = default;
    TempSoundEffectResource(std::string name, s32 vol, s32 minPitch, s32 maxPitch, std::vector<SoundEffectResourceLocation> soundBanks)
        : mResourceName(name)
    {
        mVolume = vol;
        mMinPitch = minPitch;
        mMaxPitch = maxPitch;
        mSoundBanks = soundBanks;
    }

    std::string mResourceName;
};

class TempMusicResource : public MusicResource
{
public:
    std::string mResourceName;
};

class TempSoundResources
{
public:
    std::vector<TempMusicResource> mMusics;
};

class SoundResourcesDumper : public IDataInspector
{
public:
    explicit SoundResourcesDumper(class ResourceLocator& locator);
    virtual void HandleLvl(eDataSetType eType, const std::string& lvlName, std::unique_ptr<Oddlib::LvlArchive> lvl) override;
    virtual void OnFinished() override;
    static std::string MatchIdAnyWhere(u32 id);
private:
    void CompileSeqIds();
    void HandleBsq(eDataSetType eType, const std::map<u32, std::string>& seqNames, const Oddlib::LvlArchive::File& bsq, std::vector<const Oddlib::LvlArchive::File*>& vhs);
    void HandleUnknownSeq(eDataSetType eType, const Oddlib::LvlArchive::FileChunk& seq);
    void RemoveLoadingVhs(std::vector<const Oddlib::LvlArchive::File*>& vhs);

    void RemoveBadSoundBanks();
    void SplitSEQs();
    void RenameSEQs();
    void RemoveSEQsWithNoSoundBanks();

    void RemoveSoundBanksThatDontMatchPrimarySample();
    void MergeToFinalResources();

    TempMusicResource* Exists(std::vector<TempMusicResource>& all, const TempMusicResource& res);
    bool Exists(const std::vector<SoundBankLocation>& all, const SoundBankLocation& loc);

    TempSoundResources mTempResources;
    SoundResources mFinalResources;
    void PopulateSoundEffectSoundBanks();
    void RemoveBrokenSoundEffectSoundBanks();

    ResourceLocator& mLocator;
    void GenerateThemes();
};
