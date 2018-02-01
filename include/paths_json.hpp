#pragma once

#include <string>
#include <map>
#include <vector>
#include "types.hpp"
#include <rapidjson/fwd.h>

class PathsJson
{
public:
    struct PathLocation
    {
        std::string mDataSetName;
        std::string mDataSetFileName;
    };

    struct PathMapping
    {
        u32 mId;
        u32 mCollisionOffset;
        u32 mIndexTableOffset;
        u32 mObjectOffset;
        u32 mNumberOfScreensX;
        u32 mNumberOfScreensY;
        std::string mMusicTheme;
        std::vector<PathLocation> mLocations;

        const PathLocation* Find(const std::string& dataSetName) const;
    };

    // Given BAPATH_1 it will try to find the matching record
    const PathMapping* FindPath(const std::string& resourceName);

    void FromJson(rapidjson::Document& doc);

    // For debug UI, hitting next path will keep calling this to loop the whole
    // collection of known paths.
    std::string NextPathName(s32& idx);

    // For debug UI, displays a list of paths to load
    std::map<std::string, PathMapping> Map() const;

private:
    void FromJson(const rapidjson::Value& obj);

    std::map<std::string, PathMapping> mPathMaps;
};

