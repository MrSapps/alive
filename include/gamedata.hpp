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

    typedef std::map<std::string, std::vector<FmvSection>> FmvData;

    struct PathEntry
    {
        Uint32 mPathChunkId;
        Uint32 mNumberOfCollisionItems;
        Uint32 mObjectIndexTableOffset;
        Uint32 mObjectDataOffset;
        Uint32 mMapXSize;
        Uint32 mMapYSize;
    };

    typedef std::map<std::string, std::vector<PathEntry>> PathDb;

    const FmvData Fmvs() const
    {
        return mFmvData;
    }

    const PathDb& Paths() const
    {
        return mPathDb;
    }

private:
    bool LoadFmvData(FileSystem& fs);
    void AddPcToPsxFmvNameMappings(FileSystem& fs);
    bool LoadPathDb(FileSystem& fs);
private:

    FmvData mFmvData;
    PathDb mPathDb;
};
