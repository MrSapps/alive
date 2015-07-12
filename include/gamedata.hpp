#pragma once

#include <string>
#include <map>
#include <vector>

class GameData
{
public:
    GameData();
    ~GameData();
    bool Init(std::string basePath);
    const std::map<std::string, std::vector<std::string>>& Fmvs() const
    {
        return mFmvData;
    }

private:
    bool LoadFmvData(std::string basePath);

private:
    std::map<std::string, std::vector<std::string>> mFmvData;
};
