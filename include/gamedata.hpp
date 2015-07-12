#pragma once

#include <string>
#include <vector>

class GameData
{
public:
    GameData();
    ~GameData();
    bool Init(std::string basePath);
    const std::vector<std::string>& Fmvs() const
    {
        return allFmvs;
    }
private:
    std::vector<std::string> allFmvs;
};
