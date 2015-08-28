#pragma once

#include <string>
#include <map>
#include <vector>

class FileSystem;

class GameData
{
public:
    GameData();
    ~GameData();
    bool Init(FileSystem& fs);
    const std::map<std::string, std::vector<std::string>>& Fmvs() const
    {
        return mFmvData;
    }

private:
    bool LoadFmvData(FileSystem& fs);

private:
    std::map<std::string, std::vector<std::string>> mFmvData;
};
