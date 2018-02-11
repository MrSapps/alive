#include "paths_json.hpp"
#include "oddlib/stream.hpp"
#include "proxy_rapidjson.hpp"
#include <jsonxx/jsonxx.h>

const PathsJson::PathLocation* PathsJson::PathMapping::Find(const std::string& dataSetName) const
{
    for (const PathLocation& location : mLocations)
    {
        if (location.mDataSetName == dataSetName)
        {
            return &location;
        }
    }
    return nullptr;
}

const PathsJson::PathMapping* PathsJson::FindPath(const std::string& resourceName)
{
    const auto it = mPathMaps.find(resourceName);
    if (it != std::end(mPathMaps))
    {
        return &it->second;
    }
    return nullptr;
}

static inline bool ReadStringIf(const rapidjson::Value& obj, const char* key, std::string& out)
{
    if (obj.HasMember(key))
    {
        out = obj[key].GetString();
        return true;
    }
    return false;
}

template<class T>
static inline void ReadIntIf(const rapidjson::Value& obj, const char* key, T& out)
{
    if (obj.HasMember(key))
    {
        out = obj[key].GetInt();
    }
}

void PathsJson::PathMapping::DeSerialize(const rapidjson::Value& obj)
{
    mId = obj["id"].GetInt();
    mCollisionOffset = obj["collision_offset"].GetInt();
    mIndexTableOffset = obj["object_indextable_offset"].GetInt();
    mObjectOffset = obj["object_offset"].GetInt();
    mNumberOfScreensX = obj["number_of_screens_x"].GetInt();
    mNumberOfScreensY = obj["number_of_screens_y"].GetInt();

    ReadIntIf(obj, "spawn_x", mSpawnXPos);
    ReadIntIf(obj, "spawn_y", mSpawnYPos);

    const auto& locations = obj["locations"].GetArray();
    for (auto& locationRecord : locations)
    {
        const auto& dataSet = locationRecord["dataset"].GetString();
        const auto& dataSetFileName = locationRecord["file_name"].GetString();
        mLocations.push_back(PathLocation{ dataSet, dataSetFileName });
    }
}

void PathsJson::PathMapping::Serialize(jsonxx::Object& obj) const
{
    obj << "id" << mId;
    obj << "collision_offset" << mCollisionOffset;
    obj << "object_indextable_offset" << mIndexTableOffset;
    obj << "object_offset" << mObjectOffset;
    obj << "number_of_screens_x" << mNumberOfScreensX;
    obj << "number_of_screens_y" << mNumberOfScreensY;

    if (mSpawnXPos != -1 && mSpawnYPos != -1)
    {
        obj << "spawn_x" << mSpawnXPos;
        obj << "spawn_y" << mSpawnYPos;
    }

    if (mTheme)
    {
        obj << "theme" << mTheme->mName;
    }

    jsonxx::Array locationsArray;
    for (const auto& location : mLocations)
    {
        jsonxx::Object locationObject;
        locationObject << "dataset" << location.mDataSetName;
        locationObject << "file_name" << location.mDataSetFileName;
        locationsArray << locationObject;
    }

    obj << "locations" << locationsArray;
}

void PathsJson::PathTheme::DeSerialize(const rapidjson::Value& obj)
{
    ReadStringIf(obj, "name", mName);
    ReadStringIf(obj, "music_theme", mMusicTheme);
    ReadStringIf(obj, "glukkon_skin", mGlukkonSkin);
    ReadStringIf(obj, "lift_skin", mLiftSkin);
    ReadStringIf(obj, "slam_door_skin", mSlamDoorSkin);
    ReadStringIf(obj, "door_skin", mDoorSkin);
}

static inline void WriteStringIf(jsonxx::Object& obj, const std::string& keyName, const std::string& str)
{
    if (!str.empty())
    {
        obj << keyName << str;
    }
}

void PathsJson::PathTheme::Serialize(jsonxx::Object& obj) const
{
    obj << "name" << mName;
    WriteStringIf(obj, "music_theme", mMusicTheme);
    WriteStringIf(obj, "glukkon_skin", mGlukkonSkin);
    WriteStringIf(obj, "lift_skin", mLiftSkin);
    WriteStringIf(obj, "slam_door_skin", mSlamDoorSkin);
    WriteStringIf(obj, "door_skin", mDoorSkin);
}

void PathsJson::Serialize(const std::string& fileName)
{
    jsonxx::Object rootObject;

    jsonxx::Array themesArray;
    for (const auto& theme : mPathThemes)
    {
        jsonxx::Object themeObject;
        theme.second->Serialize(themeObject);
        themesArray << themeObject;
    }
    rootObject << "path_themes" << themesArray;

    jsonxx::Array pathsArray;
    for (const auto& path : mPathMaps)
    {
        jsonxx::Object pathObject;
        pathObject << "resource_name" << path.first;

        path.second.Serialize(pathObject);

        pathsArray << pathObject;
    }

    rootObject << "paths" << pathsArray;

    Oddlib::FileStream fs(fileName, Oddlib::IStream::ReadMode::ReadWrite);
    fs.Write(rootObject.json());
}

void PathsJson::DeSerialize(const std::string& json)
{
    rapidjson::Document doc;
    doc.Parse(json.c_str());

    const auto& docRootObj = doc.GetObject();
    
    // Read the themes first
    for (auto& it : docRootObj)
    {
        if (it.name == "path_themes")
        {
            const rapidjson::Value::Array& pathsArray = it.value.GetArray();
            for (const rapidjson::Value& obj : pathsArray)
            {
                PathTheme::Ptr theme = std::make_unique<PathTheme>();
                theme->DeSerialize(obj);

                const auto& name = obj["name"].GetString();
                mPathThemes[name] = std::move(theme);
            }
        }
    }

    // Now read the path records
    for (auto& it : docRootObj)
    {
        if (it.name == "paths")
        {
            const rapidjson::Value::Array& pathsArray = it.value.GetArray();
            for (const rapidjson::Value& obj : pathsArray)
            {
                PathMapping mapping;
                mapping.DeSerialize(obj);

                const auto& name = obj["resource_name"].GetString();
              
                // If there is a theme set a pointer to the theme rather than just
                // storing the theme name itself

                std::string theme;
                if (ReadStringIf(obj, "theme", theme))
                {
                    auto themeIt = mPathThemes.find(theme);
                    if (themeIt != std::end(mPathThemes))
                    {
                        mapping.mTheme = themeIt->second.get();
                    }
                }

                mPathMaps[name] = mapping;
            }
        }
    }
}

std::string PathsJson::NextPathName(s32& idx)
{
    // Loop back to the start
    if (static_cast<size_t>(idx) > mPathMaps.size())
    {
        idx = 0;
    }

    // Find in the map by index
    int i = 0;
    for (auto& it : mPathMaps)
    {
        if (i == idx)
        {
            idx++;
            return it.first;
        }
        i++;

    }

    // Impossible unless the map is empty
    return "";
}

PathsJson::NameToPathMappingTable PathsJson::Map() const
{
    return mPathMaps;
}
