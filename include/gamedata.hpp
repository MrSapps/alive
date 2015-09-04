#pragma once

#include <string>
#include <map>
#include <vector>
#include "SDL_types.h"

class FileSystem;

class GameData
{
public:
    GameData();
    ~GameData();
    bool Init(FileSystem& fs);

    struct FmvSection
    {
        std::string mPsxFileName;
        Uint32 mStartSector;
        Uint32 mNumberOfSectors;
    };  
    
    const std::map<std::string, std::vector<FmvSection>> Fmvs() const
    {
        return mFmvData;
    }
private:
    bool LoadFmvData(FileSystem& fs);

private:

    std::map<std::string, std::vector<FmvSection>> mFmvData;
};
