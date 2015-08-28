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
        jsonxx::Object& fmvObj = rootJsonObject.get<jsonxx::Object>("FMVS");
        for (auto& v : fmvObj.kv_map())
        {
            std::string arrayName = v.first;

            std::vector<std::string> fmvs;
            jsonxx::Array& ar = fmvObj.get<jsonxx::Array>(v.first);
            for (size_t i = 0; i < ar.size(); i++)
            {
                jsonxx::String s = ar.get<jsonxx::String>(i);
                fmvs.emplace_back(s);
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
