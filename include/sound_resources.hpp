#pragma once

#include <string>
#include <vector>
#include <set>
#include <map>
#include "types.hpp"
#include "proxy_rapidjson.hpp"

class SoundBankLocation
{
public:
    std::string mName;
    std::string mDataSetName;
    std::string mSeqFileName;
    std::string mSoundBankName;
};

class MusicResource
{
public:
    u32 mResourceId;
    std::set<std::string> mSoundBanks;
};

class SoundEffectResourceLocation
{
public:
    // Because each sound of sound banks can be in another data set 
    // the program/tone can also change between them
    s32 mProgram;
    s32 mTone;
    std::set<std::string> mSoundBanks;
};

class SoundEffectResource
{
public:
    s32 mVolume;
    s32 mMinPitch;
    s32 mMaxPitch;
    std::vector<SoundEffectResourceLocation> mSoundBanks;
};

class SoundResource
{
public:
    std::string mResourceName;
    bool mIsCacheResident;
    MusicResource mMusic;
    SoundEffectResource mSoundEffect;
    std::string mComment;
};


class MusicThemeEntry
{
public:
    std::string mMusicName;
    s32 mLoopCount;
};

class MusicTheme
{
public:
    std::string mName;
    std::map<std::string, std::vector<MusicThemeEntry>> mEntries;

    const std::vector<MusicThemeEntry>* FindEntry(const char* entryName) const;
};

class SoundResources
{
public:
    std::vector<SoundResource> mSounds;
    std::vector<SoundBankLocation> mSoundBanks;
    std::vector<MusicTheme> mThemes;

    void Parse(const std::string& json);
    void Dump(const std::string& fileName);
    const SoundResource* FindSound(const char* resourceName) const;
    const SoundBankLocation* FindSoundBank(const std::string& soundBank) const;
    const MusicTheme* FindMusicTheme(const char* themeName) const;
private:
    void ParseSEQ(SoundResource& res, const rapidjson::Value& obj);
    void ParseSample(SoundResource& res, const rapidjson::Value& obj);
    void ParseSoundBanks(const rapidjson::Value& obj);
    void ParseThemes(const rapidjson::Value& obj);
};
