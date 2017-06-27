#include "sound_resources.hpp"
#include "jsonxx/jsonxx.h"
#include "logger.hpp"
#include <fstream>

const std::vector<MusicThemeEntry>* MusicTheme::FindEntry(const char* entryName) const
{
    auto it = mEntries.find(entryName);
    if (it != std::end(mEntries))
    {
        return &it->second;
    }
    return nullptr;
}

void SoundResources::ParseSEQ(SoundResource& res, const rapidjson::Value& obj)
{
    res.mMusic.mResourceId = obj["resource_id"].GetInt();

    const auto& soundBanksArray = obj["sound_banks"].GetArray();
    for (const auto& soundBank : soundBanksArray)
    {
        res.mMusic.mSoundBanks.insert(soundBank.GetString());
    }
}

void SoundResources::ParseSample(SoundResource& res, const rapidjson::Value& obj)
{

    res.mSoundEffect.mVolume = obj["volume"].GetInt();
    res.mSoundEffect.mMinPitch = obj["min_pitch"].GetInt();
    res.mSoundEffect.mMaxPitch = obj["max_pitch"].GetInt();

    const auto& locationsArray = obj["locations"].GetArray();
    for (const auto& locationObject : locationsArray)
    {
        SoundEffectResourceLocation location;
        location.mProgram = locationObject["program"].GetInt();
        location.mTone = locationObject["tone"].GetInt();
        const auto& soundBanksArray = locationObject["sound_banks"].GetArray();
        for (const auto& soundBank : soundBanksArray)
        {
            location.mSoundBanks.insert(soundBank.GetString());
        }
        res.mSoundEffect.mSoundBanks.push_back(location);
    }
}

void SoundResources::ParseSoundBanks(const rapidjson::Value& obj)
{
    SoundBankLocation location;
    location.mDataSetName = obj["data_set"].GetString();
    location.mName = obj["name"].GetString();
    location.mSoundBankName = obj["vab_name"].GetString();
    location.mSeqFileName = obj["bsq_name"].GetString();
    mSoundBanks.push_back(location);
}

void SoundResources::ParseThemes(const rapidjson::Value& obj)
{
    MusicTheme musicTheme;
    musicTheme.mName = obj["name"].GetString();

    for (auto it = obj.MemberBegin(); it != obj.MemberEnd(); it++)
    {
        if (it->name != "name")
        {
            std::vector<MusicThemeEntry> entries;
            const auto& entriesArray = it->value.GetArray();
            for (const auto& entriesArrayItem : entriesArray)
            {
                MusicThemeEntry entry;
                entry.mMusicName = entriesArrayItem.GetString();
                entries.push_back(entry);
            }
            musicTheme.mEntries[it->name.GetString()] = std::move(entries);
        }
       
    }

    mThemes.push_back(musicTheme);
}

void SoundResources::Parse(const std::string& json)
{
    TRACE_ENTRYEXIT;

    rapidjson::Document document;
    document.Parse(json.c_str());

    if (document.HasParseError())
    {
        return;
    }

    const auto& docRootArray = document.GetArray();
    for (auto& it : docRootArray)
    {
        if (it.HasMember("sound_resources"))
        {
            const auto& soundResourceArray = it["sound_resources"].GetArray();
            for (auto& obj : soundResourceArray)
            {
                SoundResource soundRes;
                if (obj.HasMember("seq"))
                {
                    const auto& seqObj = obj["seq"].GetObject();
                    ParseSEQ(soundRes, seqObj);
                }

                if (obj.HasMember("sample"))
                {
                    const auto& sampleObj = obj["sample"].GetObject();
                    ParseSample(soundRes, sampleObj);
                }

                soundRes.mResourceName = obj["resource_name"].GetString();

                mSounds.push_back(soundRes);
            }
        }

        if (it.HasMember("sound_banks"))
        {
            const auto& soundBanksArray = it["sound_banks"].GetArray();
            for (auto& obj : soundBanksArray)
            {
                ParseSoundBanks(obj);
            }
        }

        if (it.HasMember("themes"))
        {
            const auto& themesArray = it["themes"].GetArray();
            for (auto& obj : themesArray)
            {
                ParseThemes(obj);
            }
        }
    }
}

void SoundResources::Dump(const std::string& fileName)
{
    jsonxx::Array soundResources;

    jsonxx::Array soundsArray;

    for (const SoundResource& sndRes : mSounds)
    {
        jsonxx::Object soundResourceObject;

        if (!sndRes.mMusic.mSoundBanks.empty())
        {
            jsonxx::Array musicSoundBanks;
            for (const std::string& sb : sndRes.mMusic.mSoundBanks)
            {
                musicSoundBanks << sb;
            }

            jsonxx::Object music;
            music
                << "resource_id"
                << sndRes.mMusic.mResourceId
                << "sound_banks"
                << musicSoundBanks;

            soundResourceObject << "seq" << music;
        }

        if (!sndRes.mSoundEffect.mSoundBanks.empty())
        {
            jsonxx::Array locationsArray;
            for (const SoundEffectResourceLocation& loc : sndRes.mSoundEffect.mSoundBanks)
            {

                jsonxx::Array soundEffectSoundBanks;
                for (const std::string& sb : loc.mSoundBanks)
                {
                    soundEffectSoundBanks << sb;
                }

                jsonxx::Object locationsObject;
                locationsObject
                    << "tone"
                    << loc.mTone
                    << "program"
                    << loc.mProgram
                    << "sound_banks"
                    << soundEffectSoundBanks;

                locationsArray << locationsObject;
            }

            jsonxx::Object soundEffect;
            soundEffect
                << "max_pitch"
                << sndRes.mSoundEffect.mMaxPitch
                << "min_pitch"
                << sndRes.mSoundEffect.mMinPitch
                << "volume"
                << sndRes.mSoundEffect.mVolume
                << "locations"
                << locationsArray;

            soundResourceObject << "sample" << soundEffect;
        }

        soundResourceObject << "resource_name" << sndRes.mResourceName;
        soundsArray << soundResourceObject;
    }

    jsonxx::Object soundResourcesObj;
    soundResourcesObj << "sound_resources" << soundsArray;

    soundResources << soundResourcesObj;

    // Sound banks
    jsonxx::Array soundBanks;
    for (const SoundBankLocation& soundBankRes : mSoundBanks)
    {
        jsonxx::Object soundBank;
        soundBank
            << "name" << soundBankRes.mName
            << "data_set" << soundBankRes.mDataSetName
            << "bsq_name" << soundBankRes.mSeqFileName
            << "vab_name" << soundBankRes.mSoundBankName;

        soundBanks << soundBank;
    }

    jsonxx::Object soundBanksObj;
    soundBanksObj << "sound_banks" << soundBanks;
    soundResources << soundBanksObj;

    // Themes
    jsonxx::Array themesArray;
    for (const MusicTheme& theme : mThemes)
    {
        jsonxx::Object themeObj;
        themeObj << "name" << theme.mName;
        for (const auto& themePair : theme.mEntries)
        {
            jsonxx::Array entriesArray;
            for (const MusicThemeEntry& entry : themePair.second)
            {
                entriesArray << entry.mMusicName;
            }
            themeObj << themePair.first << entriesArray;
        }
        themesArray << themeObj;
    }

    jsonxx::Object themesObj;
    themesObj << "themes" << themesArray;
    soundResources << themesObj;

    std::ofstream jsonFile(fileName);
    if (!jsonFile.is_open())
    {
        abort();
    }
    jsonFile << soundResources.json().c_str() << std::endl;
}

const SoundResource* SoundResources::FindSound(const char* resourceName) const
{
    for (const SoundResource& mr : mSounds)
    {
        if (mr.mResourceName == resourceName)
        {
            return &mr;
        }
    }
    return nullptr;
}

const SoundBankLocation* SoundResources::FindSoundBank(const std::string& soundBank) const
{
    for (const SoundBankLocation& sbl : mSoundBanks)
    {
        if (sbl.mName == soundBank)
        {
            return &sbl;
        }
    }
    return nullptr;
}

const MusicTheme* SoundResources::FindMusicTheme(const char* themeName) const
{
    for (const MusicTheme& mt : mThemes)
    {
        if (mt.mName == themeName)
        {
            return &mt;
        }
    }
    return nullptr;
}
