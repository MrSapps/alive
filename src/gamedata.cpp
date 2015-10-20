#include "gamedata.hpp"
#include "jsonxx/jsonxx.h"
#include <fstream>
#include "alive_version.h"
#include "SDL.h"
#include "filesystem.hpp"
#include "oddlib/stream.hpp"

GameData::GameData()
{

}

GameData::~GameData()
{

}

bool GameData::LoadFmvDb(FileSystem& fs)
{
    auto stream = fs.GameData().Open("data/fmvdb.json");
    std::string jsonFileContents = stream->LoadAllToString();

    jsonxx::Object rootJsonObject;
    rootJsonObject.parse(jsonFileContents);
    if (rootJsonObject.has<jsonxx::Object>("FMVS"))
    {
        const jsonxx::Object& fmvObj = rootJsonObject.get<jsonxx::Object>("FMVS");
        for (auto& v : fmvObj.kv_map())
        {
            // Group of FMV's name, e.g. "AbesExoddusPcDemo"
            //const std::string& gameName = v.first;

            const jsonxx::Array& ar = fmvObj.get<jsonxx::Array>(v.first);
            for (Uint32 i = 0; i < static_cast<Uint32>(ar.size()); i++)
            {
                // Read out the record
                FmvSection section = {};
                std::string pcFileName;
                if (ar.has<jsonxx::String>(i))
                {
                    // Just a file name
                    pcFileName = ar.get<jsonxx::String>(i);
                    auto it = mFmvDb.find(pcFileName);
                    if (it == std::end(mFmvDb))
                    {
                        mFmvDb[pcFileName] = std::vector<FmvSection>();
                        it = mFmvDb.find(pcFileName);
                    }
                }
                else if (ar.has<jsonxx::Object>(i))
                {
                    // Maps a PSX file name to a PC file name - and what part of the PSX
                    // file contains the PC file. As PSX movies are many movies in one file.
                    const jsonxx::Object& subFmvObj = ar.get<jsonxx::Object>(i);
                    section.mPsxFileName = subFmvObj.get<jsonxx::String>("file");
                    const jsonxx::Array& containsArray = subFmvObj.get<jsonxx::Array>("contains");

                    for (Uint32 j = 0; j < static_cast<Uint32>(containsArray.size()); j++)
                    {
                        const jsonxx::Object& subFmvSettings = containsArray.get<jsonxx::Object>(j);;
                        pcFileName  = subFmvSettings.get<jsonxx::String>("name");
                        section.mStartSector = static_cast<Uint32>(subFmvSettings.get<jsonxx::Number>("start_sector"));
                        section.mNumberOfSectors = static_cast<Uint32>(subFmvSettings.get<jsonxx::Number>("number_of_sectors"));

                        // Grab a vector for the fmv name e.g "whatever.ddv"
                        auto it = mFmvDb.find(pcFileName);
                        if (it == std::end(mFmvDb))
                        {
                            mFmvDb[pcFileName] = std::vector<FmvSection>();
                            it = mFmvDb.find(pcFileName);
                        }

                        it->second.emplace_back(section);
                    }
                }

               

            }
        }
    }

    return true;
}

void GameData::AddPcToPsxFmvNameMappings(FileSystem& fs)
{
    for (const auto& fmvData : mFmvDb)
    {
        for (const FmvSection& fmvSection : fmvData.second)
        {
            fs.ResourcePaths().AddPcToPsxMapping(fmvData.first, fmvSection.mPsxFileName);
        }
    }
}

bool GameData::LoadPathDb(FileSystem& fs)
{
    auto stream = fs.GameData().Open("data/pathdb.json");
    std::string jsonFileContents = stream->LoadAllToString();

    jsonxx::Object rootJsonObject;
    rootJsonObject.parse(jsonFileContents);
    if (rootJsonObject.has<jsonxx::Array>("lvls"))
    {
        const auto& lvls = rootJsonObject.get<jsonxx::Array>("lvls");
        for (Uint32 i = 0; i < static_cast<Uint32>(lvls.size()); i++)
        {
            const auto lvlEntry = lvls.get<jsonxx::Object>(i);
            const auto pathBndFileName = lvlEntry.get<jsonxx::String>("file_name");
            auto& pathDbIterator = mPathDb[pathBndFileName];
            const auto paths = lvlEntry.get<jsonxx::Array>("paths");
            for (Uint32 j = 0; j < static_cast<Uint32>(paths.size()); j++)
            {
                const auto pathObj = paths.get<jsonxx::Object>(j);
                pathDbIterator.emplace_back(PathEntry
                {
                    static_cast<Uint32>(pathObj.get<jsonxx::Number>("id")),
                    static_cast<Uint32>(pathObj.get<jsonxx::Number>("collision")),
                    static_cast<Uint32>(pathObj.get<jsonxx::Number>("index")),
                    static_cast<Uint32>(pathObj.get<jsonxx::Number>("object")),
                    static_cast<Uint32>(pathObj.get<jsonxx::Number>("x")),
                    static_cast<Uint32>(pathObj.get<jsonxx::Number>("y"))
                });
            }
        }
    }

    return true;
}

bool GameData::LoadLvlDb(FileSystem& fs)
{
    auto stream = fs.GameData().Open("data/lvldb.json");
    std::string jsonFileContents = stream->LoadAllToString();

    jsonxx::Object rootJsonObject;
    rootJsonObject.parse(jsonFileContents);
    if (rootJsonObject.has<jsonxx::Array>("lvls"))
    {
        const auto& lvls = rootJsonObject.get<jsonxx::Array>("lvls");
        for (Uint32 i = 0; i < static_cast<Uint32>(lvls.size()); i++)
        {
            const auto lvlObj = lvls.get<jsonxx::Object>(i);
            const auto pcName = lvlObj.get<jsonxx::String>("name");
            const auto altNames = lvlObj.get<jsonxx::Array>("alt_names");
            for (Uint32 j = 0; j < static_cast<Uint32>(altNames.size()); j++)
            {
                const auto altName = altNames.get<jsonxx::String>(j);
                fs.ResourcePaths().AddPcToPsxMapping(pcName, altName);
            }
        }
    }
    return true;
}

bool GameData::Init(FileSystem& fs)
{
    if (!LoadFmvDb(fs))
    {
        return false;
    }

    AddPcToPsxFmvNameMappings(fs);

    if (!LoadPathDb(fs))
    {
        return false;
    }

    if (!LoadLvlDb(fs))
    {
        return false;
    }

    return true;
}
