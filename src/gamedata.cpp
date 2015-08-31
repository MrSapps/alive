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

bool GameData::LoadFmvData(FileSystem& fs)
{
    auto stream = fs.Open("data/videos.json");
    std::string jsonFileContents = stream->LoadAllToString();

    jsonxx::Object rootJsonObject;
    rootJsonObject.parse(jsonFileContents);
    if (rootJsonObject.has<jsonxx::Object>("FMVS"))
    {
        const jsonxx::Object& fmvObj = rootJsonObject.get<jsonxx::Object>("FMVS");
        for (auto& v : fmvObj.kv_map())
        {
            // Group of FMV's name, e.g. "AbesExoddusPcDemo"
            const std::string& gameName = v.first;


            const jsonxx::Array& ar = fmvObj.get<jsonxx::Array>(v.first);
            for (size_t i = 0; i < ar.size(); i++)
            {
                // Read out the record
                FmvSection section = {};
                bool isSubEntry = false;
                std::string pcFileName;
                if (ar.has<jsonxx::String>(i))
                {
                    // Just a file name
                    pcFileName = ar.get<jsonxx::String>(i);
                    auto it = mFmvData.find(pcFileName);
                    if (it == std::end(mFmvData))
                    {
                        mFmvData[pcFileName] = std::vector<FmvSection>();
                        it = mFmvData.find(pcFileName);
                    }
                }
                else if (ar.has<jsonxx::Object>(i))
                {
                    isSubEntry = true;

                    // Maps a PSX file name to a PC file name - and what part of the PSX
                    // file contains the PC file. As PSX movies are many movies in one file.
                    const jsonxx::Object& subFmvObj = ar.get<jsonxx::Object>(i);
                    section.mPsxFileName = subFmvObj.get<jsonxx::String>("file");
                    const jsonxx::Array& containsArray = subFmvObj.get<jsonxx::Array>("contains");

                    for (size_t j = 0; j < containsArray.size(); j++)
                    {
                        const jsonxx::Object& subFmvSettings = containsArray.get<jsonxx::Object>(j);;
                        pcFileName  = subFmvSettings.get<jsonxx::String>("name");
                        section.mStart = static_cast<Uint32>(subFmvSettings.get<jsonxx::Number>("start"));
                        section.mEnd = static_cast<Uint32>(subFmvSettings.get<jsonxx::Number>("end"));

                        // Grab a vector for the fmv name e.g "whatever.ddv"
                        auto it = mFmvData.find(pcFileName);
                        if (it == std::end(mFmvData))
                        {
                            mFmvData[pcFileName] = std::vector<FmvSection>();
                            it = mFmvData.find(pcFileName);
                        }

                        it->second.emplace_back(section);
                    }
                }

               

            }
        }
    }

    return true;
}

bool GameData::Init(FileSystem& fs)
{
    if (!LoadFmvData(fs))
    {
        return false;
    }
    return true;
}
