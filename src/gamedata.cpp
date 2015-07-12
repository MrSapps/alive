#include "gamedata.hpp"
#include "jsonxx/jsonxx.h"
#include <fstream>
#include "alive_version.h"
#include "SDL.h"

GameData::GameData()
{

}

GameData::~GameData()
{

}

bool GameData::LoadFmvData(std::string basePath)
{
    jsonxx::Object rootJsonObject;
    std::ifstream tmpStream(basePath + "data/videos.json");
    if (!tmpStream)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
            ALIVE_VERSION_NAME_STR  " Missing file",
            (basePath + "data/videos.json is missing.").c_str(),
            NULL);
        return false;
    }
    std::string jsonFileContents((std::istreambuf_iterator<char>(tmpStream)), std::istreambuf_iterator<char>());


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

bool GameData::Init(std::string basePath)
{
    if (!LoadFmvData(basePath))
    {
        return false;
    }
    return true;
}
