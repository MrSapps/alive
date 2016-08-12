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
        u32 mStartSector;
        u32 mNumberOfSectors;
    };  

    typedef std::map<std::string, std::vector<FmvSection>> FmvDb;

    struct PathEntry
    {
        u32 mPathChunkId;
        u32 mCollisionDataOffset;
        u32 mObjectIndexTableOffset;
        u32 mObjectDataOffset;
        u32 mMapXSize;
        u32 mMapYSize;
        bool mIsAo;
    };

    typedef std::map<std::string, std::vector<PathEntry>> PathDb;

    const FmvDb Fmvs() const
    {
        return mFmvDb;
    }

    const PathDb& Paths() const
    {
        return mPathDb;
    }

private:
    bool LoadFmvDb(FileSystem& fs);
    void AddPcToPsxFmvNameMappings(FileSystem& fs);
    bool LoadPathDb(FileSystem& fs);
    bool LoadLvlDb(FileSystem& fs);
private:

    FmvDb mFmvDb;
    PathDb mPathDb;
};
