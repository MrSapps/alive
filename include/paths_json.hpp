#pragma once

#include <string>
#include <map>
#include <vector>
#include <memory>
#include "types.hpp"
#include <rapidjson/fwd.h>

namespace jsonxx 
{ 
    class Object;
}

class PathsJson
{
public:
    struct PathLocation
    {
        std::string mDataSetName;
        std::string mDataSetFileName;
    };

    struct PathTheme
    {
        std::string mName;
        std::string mMusicTheme;
        std::string mGlukkonSkin;
        std::string mLiftSkin;
        std::string mSlamDoorSkin;
        std::string mDoorSkin;
        using Ptr = std::unique_ptr<PathTheme>;
        void DeSerialize(const rapidjson::Value& obj);
        void Serialize(jsonxx::Object& obj) const;
    };
    using NameToPathThemeTable = std::map<std::string, PathTheme::Ptr>; // Ptr so PathMapping's pointer won't dangle

    struct PathMapping
    {
        u32 mId = 0;
        u32 mCollisionOffset = 0;
        u32 mIndexTableOffset = 0;
        u32 mObjectOffset = 0;
        u32 mNumberOfScreensX = 0;
        u32 mNumberOfScreensY = 0;
        s32 mSpawnXPos = -1;
        s32 mSpawnYPos = -1;
        std::vector<PathLocation> mLocations;
        PathTheme* mTheme = nullptr;
        const PathLocation* Find(const std::string& dataSetName) const;
        void DeSerialize(const rapidjson::Value& obj);
        void Serialize(jsonxx::Object& obj) const;
    };

    using NameToPathMappingTable = std::map<std::string, PathMapping>;

    // Given BAPATH_1 it will try to find the matching record
    const PathMapping* FindPath(const std::string& resourceName);

    void DeSerialize(const std::string& json);
    void Serialize(const std::string& fileName);

    // For debug UI, hitting next path will keep calling this to loop the whole
    // collection of known paths.
    std::string NextPathName(s32& idx);

    // For debug UI, displays a list of paths to load
    NameToPathMappingTable Map() const;

protected:
    NameToPathMappingTable mPathMaps;
    NameToPathThemeTable mPathThemes;
};

