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
            const std::string& arrayName = v.first;

            std::vector<std::string> fmvs;
            const jsonxx::Array& ar = fmvObj.get<jsonxx::Array>(v.first);
            for (size_t i = 0; i < ar.size(); i++)
            {
                if (ar.has<jsonxx::String>(i))
                {
                    const jsonxx::String& s = ar.get<jsonxx::String>(i);
                    fmvs.emplace_back(s);
                }
                else if (ar.has<jsonxx::Object>(i))
                {
                    const jsonxx::Object& subFmvObj = ar.get<jsonxx::Object>(i);
                    const std::string& file = subFmvObj.get<jsonxx::String>("file");
                    const jsonxx::Array& containsArray = subFmvObj.get<jsonxx::Array>("contains");

                    for (size_t j = 0; j < containsArray.size(); j++)
                    {
                        const jsonxx::Object& subFmvSettings = containsArray.get<jsonxx::Object>(j);
                        const std::string& mappedName = subFmvSettings.get<jsonxx::String>("name");
                        const Uint32 startSector = static_cast<Uint32>(subFmvSettings.get<jsonxx::Number>("start"));
                        const Uint32 endSector = static_cast<Uint32>(subFmvSettings.get<jsonxx::Number>("end"));

                    }

                }
            }
            mFmvData[v.first] = std::move(fmvs);
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
