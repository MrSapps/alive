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

    typedef std::map<std::string, std::vector<FmvSection>> FmvDb;

    struct PathEntry
    {
        Uint32 mPathChunkId;
        Uint32 mCollisionDataOffset;
        Uint32 mObjectIndexTableOffset;
        Uint32 mObjectDataOffset;
        Uint32 mMapXSize;
        Uint32 mMapYSize;
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
