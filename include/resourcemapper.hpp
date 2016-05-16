#pragma once

#include "jsonxx/jsonxx.h"
#include <unordered_map>
#include <regex>
#include <map>

#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif

#include "string_util.hpp"
#include "oddlib/cdromfilesystem.hpp"

inline size_t StringHash(const char* s)
{
    // FNV hasher
    size_t result = 2166136261U;
    while (*s)
    {
        result = (16777619 * result) ^ static_cast<unsigned char>(*s);
        s++;
    }
    return result;
}

inline size_t StringHash(const std::string& s)
{
    return StringHash(s.c_str());
}

inline std::vector<Uint8> StringToVector(const std::string& str)
{
    return std::vector<Uint8>(str.begin(), str.end());
}

class IFileSystem
{
public:
    virtual ~IFileSystem() = default;
    
    virtual bool Init() { return true; }
    virtual std::unique_ptr<Oddlib::IStream> Open(const char* fileName) = 0;

    virtual std::vector<std::string> EnumerateFiles(const char* directory, const char* filter) = 0;
    virtual bool FileExists(const char* fileName) = 0;
};

class OSFileSystem : public IFileSystem
{
private:
    static bool IsDots(const std::string& name)
    {
        return name == "." || name == "..";
    }
public:

    virtual bool Init() override
    {
        auto basePath = InitBasePath();
        if (basePath.empty())
        {
            return false;
        }
        mNamedPaths["{GameDir}"] = basePath;

        // TODO: Resolve {UserDir}
        mNamedPaths["{UserDir}"] = ".";

        return true;
    }

    virtual std::unique_ptr<Oddlib::IStream> Open(const char* fileName) override
    {
        return std::make_unique<Oddlib::Stream>(ExpandPath(fileName));
    }

#ifdef _WIN32
    struct FindCloseDeleter
    {
        typedef HANDLE pointer;
        void operator()(HANDLE hFind)
        {
            if (hFind != INVALID_HANDLE_VALUE)
            {
                ::FindClose(hFind);
            }
        }
    };
    typedef std::unique_ptr<HANDLE, FindCloseDeleter> FindCloseHandle;

    virtual std::vector<std::string> EnumerateFiles(const char* directory, const char* filter) override
    {
        std::vector<std::string> ret;
        WIN32_FIND_DATA findData = {};
        const std::string dirPath = ExpandPath(directory) + "/" + filter;
        FindCloseHandle ptr(::FindFirstFile(dirPath.c_str(), &findData));
        if (ptr.get() != INVALID_HANDLE_VALUE)
        {
            do
            {
                if (!IsDots(findData.cFileName) && !(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                {
                    ret.emplace_back(findData.cFileName);
                }
            } while (::FindNextFile(ptr.get(), &findData));
            LOG_INFO(ret.size() << " items enumerated from " << dirPath);
        }
        else
        {
            LOG_ERROR("Failed to enumerate directory " << dirPath);
        }
        return ret;
    }
#else
    struct closedirDeleter
    {
        typedef DIR* pointer;
        void operator()(DIR* dir)
        {
            closedir(dir);
        }
    };
    typedef std::unique_ptr<DIR, closedirDeleter> closedirHandle;

    virtual std::vector<std::string> EnumerateFiles(const char* directory, const char* filter) override
    {
        std::vector<std::string> ret;
        const std::string dirPath = ExpandPath(directory) + "/";
        closedirHandle dir(opendir(dirPath.c_str()));
        if (dir)
        {
            dirent* ent = nullptr;
            do
            {
                const std::string strFilter(filter);
                ent = readdir(dir.get());
                if (ent)
                {
                    const std::string dirName = ent->d_name;
                    if (!IsDots(dirName) && WildCardMatcher(dirName, strFilter, true))
                    {
                        ret.emplace_back(dirName);
                        LOG_INFO("Name is " << dirName);
                    }
                }
            } while (ent);
            LOG_INFO(ret.size() << " items enumerated from " << dirPath);
        }
        else
        {
            LOG_ERROR("Failed to enumerate directory " << dirPath);
        }
        return ret;
    }
#endif

#ifdef _WIN32
    bool FileExists(const char* fileName) override
    {
        const auto name = ExpandPath(fileName);
        const DWORD dwAttrib = GetFileAttributes(name.c_str());
        return (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
    }
#else
    bool FileExists(const char* fileName) override
    {
        struct stat buffer = {};
        return stat(fileName, &buffer) == 0; 
    }
#endif

private:
    bool WildCardMatcher(const std::string& text, std::string wildcardPattern, bool caseSensitive)
    {
        // Escape all regex special chars
        EscapeRegex(wildcardPattern);

        // Convert chars '*?' back to their regex equivalents
        string_util::replace_all(wildcardPattern, "\\?", ".");
        string_util::replace_all(wildcardPattern, "\\*", ".*");

        std::regex pattern(wildcardPattern, 
            caseSensitive ? 
                std::regex_constants::ECMAScript : 
                std::regex_constants::ECMAScript | std::regex_constants::icase);

        return std::regex_match(text, pattern);
    }
    
    void EscapeRegex(std::string& regex)
    {
        string_util::replace_all(regex, "\\", "\\\\");
        string_util::replace_all(regex, "^", "\\^");
        string_util::replace_all(regex, ".", "\\.");
        string_util::replace_all(regex, "$", "\\$");
        string_util::replace_all(regex, "|", "\\|");
        string_util::replace_all(regex, "(", "\\(");
        string_util::replace_all(regex, ")", "\\)");
        string_util::replace_all(regex, "[", "\\[");
        string_util::replace_all(regex, "]", "\\]");
        string_util::replace_all(regex, "*", "\\*");
        string_util::replace_all(regex, "+", "\\+");
        string_util::replace_all(regex, "?", "\\?");
        string_util::replace_all(regex, "/", "\\/");
    }

    std::string ExpandPath(const std::string& path)
    {
        std::string ret = path;
        for (const auto& namedPath : mNamedPaths)
        {
            string_util::replace_all(ret, namedPath.first, namedPath.second);
        }
        string_util::replace_all(ret, '\\', '/');
        string_util::replace_all(ret, "//", "/");
        return ret;
    }

    std::string InitBasePath()
    {
        char* pBasePath = SDL_GetBasePath();
        std::string basePath;
        if (pBasePath)
        {
            basePath = pBasePath;
            SDL_free(pBasePath);

            // If it looks like we're running from the IDE/dev build then attempt to fix up the path to the correct location to save
            // manually setting the correct working directory.
            const bool bIsDebugPath = string_util::contains(basePath, "/alive/bin/") || string_util::contains(basePath, "\\alive\\bin\\");
            if (bIsDebugPath)
            {
                if (string_util::contains(basePath, "/alive/bin/"))
                {
                    LOG_WARNING("We appear to be running from the IDE (Linux) - fixing up basePath to be ../");
                    basePath += "../";
                }
                else
                {
                    LOG_WARNING("We appear to be running from the IDE (Win32) - fixing up basePath to be ../");
                    basePath += "..\\..\\";
                }
            }
        }
        else
        {
            LOG_ERROR("SDL_GetBasePath failed");
            return "";
        }
        LOG_INFO("basePath is " << basePath);
        string_util::replace_all(basePath, '\\', '/');
        return basePath;
    }

    std::map<std::string, std::string> mNamedPaths;
};

class CdIsoFileSystem : public IFileSystem
{
public:
    explicit CdIsoFileSystem(const char* fileName)
        : mRawCdImage(fileName)
    {

    }

    virtual ~CdIsoFileSystem() = default;

    virtual bool Init() override
    {
        // TODO
        return true;
    }

    virtual std::unique_ptr<Oddlib::IStream> Open(const char* fileName) override
    {
        return mRawCdImage.ReadFile(fileName, false);
    }

    virtual std::vector<std::string> EnumerateFiles(const char* /*directory*/, const char* /*filter*/) override
    {
        // TODO
        abort();
        //return std::vector<std::string> { };
    }

    virtual bool FileExists(const char* fileName) override
    {
        return mRawCdImage.FileExists(fileName) != -1;
    }

private:
    RawCdImage mRawCdImage;
};

// TODO
class ZipFileSystem
{
};

// TODO
class LvlFileSystem
{
};

class DataPathIdentities
{
public:
    DataPathIdentities() = default;

    DataPathIdentities(IFileSystem& fs, const char* dataSetsIdsFileName)
    {
        auto stream = fs.Open(dataSetsIdsFileName);
        if (stream)
        {
            Parse(stream->LoadAllToString());
        }
    }

    bool IsMetaPath(const std::string& id) const
    {
        return mMetaPaths.find(id) != std::end(mMetaPaths);
    }

    std::string Identify(IFileSystem& fs, const std::string& path) const
    {
        for (const auto& dataPathId : mDataPathIds)
        {
            if (MatchPathWithDataPathId(fs, dataPathId, path))
            {
                return dataPathId.first;
            }
        }
        return "";
    }

private:
    struct DataPathFiles
    {
        std::vector<std::string> mContainAnyOf;
        std::vector<std::string> mContainAllOf;
        std::vector<std::string> mMustNotContain;
    };

    std::map<std::string, DataPathFiles> mDataPathIds;

    // A meta path is one that exists just to include other paths
    // e.g AePsx includes AePsxCd1 and AxePsxCd1, but AePsx won't actually have a path as such
    std::set<std::string> mMetaPaths;

    bool DoMatchPathWithDataPathId(IFileSystem& fs, const std::pair<std::string, DataPathFiles>& dataPathId, const std::string& path) const
    {
        // Check all of the "must exist files" do exist
        for (const auto& f : dataPathId.second.mContainAllOf)
        {
            if (!fs.FileExists((path + "\\" + f).c_str()))
            {
                // Skip this dataPathId, the path doesn't match all of the "must contain"
                return false;
            }
        }

        // Check that all of the "must not exist" files don't exist
        for (const auto& f : dataPathId.second.mMustNotContain)
        {
            if (fs.FileExists((path + "\\" + f).c_str()))
            {
                // Found a file that shouldn't exist, skip to the next dataPathId
                return false;
            }
        }

        // Check we can find at least one of the "any of" set of files
        bool foundAnyOf = false;
        for (const auto& f : dataPathId.second.mContainAnyOf)
        {
            if (fs.FileExists((path + "\\" + f).c_str()))
            {
                foundAnyOf = true;
                break;
            }
        }

        // If one of the "any of" files exists or we have one or more "must exist" files
        // then this dataPathId matches path
        if (foundAnyOf || dataPathId.second.mContainAllOf.empty() == false)
        {
            return true;
        }

        return false;
    }

    bool MatchPathWithDataPathId(IFileSystem& fs, const std::pair<std::string, DataPathFiles>& dataPathId, const std::string& path) const
    {
        const bool isFile = fs.FileExists(path.c_str());
        if (isFile)
        {
            if (string_util::ends_with(path, ".bin", true))
            {
                CdIsoFileSystem cdFs(path.c_str());
                return DoMatchPathWithDataPathId(cdFs, dataPathId, "");
            }
            else if (string_util::ends_with(path, ".zip", true))
            {
                // TODO
                return false;
            }
            else
            {
                return false;
            }
        }
        else
        {
            return DoMatchPathWithDataPathId(fs, dataPathId, path);
        }
    }

    void ReadStringArray(const jsonxx::Object& jsonObject, const std::string& jsonArrayName, std::vector<std::string>& resultingStrings)
    {
        if (jsonObject.has<jsonxx::Array>(jsonArrayName))
        {
            const jsonxx::Array& stringArray = jsonObject.get<jsonxx::Array>(jsonArrayName);
            resultingStrings.reserve(stringArray.size());
            for (const auto& f : stringArray.values())
            {
                resultingStrings.emplace_back(f->get<jsonxx::String>());
            }
        }

    }

    void Parse(const std::string& json)
    {
        jsonxx::Object root;
        root.parse(json);
        if (root.has<jsonxx::Object>("data_set_ids"))
        {
            jsonxx::Object dataSetIds = root.get<jsonxx::Object>("data_set_ids");
            for (const auto& v : dataSetIds.kv_map())
            {
                const jsonxx::Object& dataSetId = dataSetIds.get<jsonxx::Object>(v.first);

                DataPathFiles dpFiles;
                ReadStringArray(dataSetId, "contains_any", dpFiles.mContainAnyOf);
                ReadStringArray(dataSetId, "contains_all", dpFiles.mContainAllOf);
                ReadStringArray(dataSetId, "not_contains", dpFiles.mMustNotContain);

                if (dpFiles.mContainAnyOf.empty() && dpFiles.mContainAllOf.empty() && dpFiles.mMustNotContain.empty())
                {
                    mMetaPaths.insert(v.first);
                }
                else
                {
                    mDataPathIds[v.first] = dpFiles;
                }
            }
        }
    }
};

class DataPaths
{
public:
    DataPaths(const DataPaths&) = delete;
    DataPaths& operator = (const DataPaths&) = delete;

    DataPaths(DataPaths&& other)
    {
        *this = std::move(other);
    }

    DataPaths& operator = (DataPaths&& other)
    {
        mIds = std::move(other.mIds);
        mPaths = std::move(other.mPaths);
        return *this;
    }

    DataPaths(IFileSystem& fs, const char* dataSetsIdsFileName, const char* dataPathFileName)
        : mIds(fs, dataSetsIdsFileName)
    {
        if (fs.FileExists(dataPathFileName))
        {
            auto stream = fs.Open(dataPathFileName);
            std::vector<std::string> paths = Parse(stream->LoadAllToString());
            for (const auto& path : paths)
            {
                std::string id = mIds.Identify(fs, path);
                auto it = mPaths.find(id);
                if (it == std::end(mPaths))
                {
                    mPaths[id] = std::vector < std::string > {path};
                }
                else
                {
                    it->second.push_back(path);
                }
            }
        }
    }

    const std::vector<std::string>& PathsFor(const std::string& id)
    {
        auto it = mPaths.find(id);
        if (it == std::end(mPaths))
        {
            return mNotFoundResult;
        }
        else
        {
            return it->second;
        }
    }

    std::vector<std::string> MissingDataSets(const std::vector<std::string>& requiredSets)
    {
        std::vector<std::string> ret;
        for (const auto& dataset : requiredSets)
        {
            if (mIds.IsMetaPath(dataset))
            {
                continue;
            }

            if (!PathsFor(dataset).empty())
            {
                continue;
            }

            ret.emplace_back(dataset);
        }
        return ret;
    }

private:
    std::map<std::string, std::vector<std::string>> mPaths;

    std::vector<std::string> Parse(const std::string& json)
    {
        std::vector<std::string> paths;
        jsonxx::Object root;
        root.parse(json);
        if (root.has<jsonxx::Array>("paths"))
        {
            jsonxx::Array pathsArray = root.get<jsonxx::Array>("paths");
            for (const auto& path : pathsArray.values())
            {
                paths.emplace_back(path->get<jsonxx::String>());
            }
        }
        return paths;
    }

    // To match to what a game def wants (AePcCd1, AoDemoPsx etc)
    // we use SLUS codes for PSX or if it contains ABEWIN.EXE etc then its AoPc.
    DataPathIdentities mIds;
    const /*static*/ std::vector<std::string> mNotFoundResult;
};

//const static std::vector<std::string> DataPaths::mNotFoundResult;

/*
// TODO: Make these known constants:
"AoPc"
"AoPcDemo"
"AoPsx"
"AoPsxDemo"
"AePc"
"AePcDemo"
"AePsxCd1"
"AePsxCd2"
"AePsxDemo"
*/

// AOPC, AOPSX, FoosMod etc
using DataSetMap = std::map<std::string, const class GameDefinition*>;
struct BuiltInAndModGameDefs
{
    std::vector< std::pair<std::string, const GameDefinition*>> gameDefs;
    std::vector< std::pair<std::string, const GameDefinition*>> modDefs;
};

class GameDefinition
{
public:
    static BuiltInAndModGameDefs SplitInToBuiltInAndMods(const DataSetMap& requiredDataSets)
    {
        BuiltInAndModGameDefs sorted;
        for (const auto& dataSetPair : requiredDataSets)
        {
            if (dataSetPair.second->IsMod())
            {
                sorted.modDefs.push_back(dataSetPair);
            }
            else
            {
                sorted.gameDefs.push_back(dataSetPair);
            }
        }
        return sorted;
    }

    static void GetDependencies(DataSetMap& requiredDataSets, std::set<std::string>& missingDataSets, const GameDefinition* gd, const std::vector<const GameDefinition*>& gds)
    {
        requiredDataSets[gd->DataSetName()] = gd;

        for (const std::string& dataSetName : gd->RequiredDataSets())
        {
            // Skip if already processed to avoid inf recursion on cycles
            if (requiredDataSets.find(dataSetName) == std::end(requiredDataSets))
            {
                bool found = false;
                for (const GameDefinition* gameDef : gds)
                {
                    if (gameDef->DataSetName() == dataSetName)
                    {
                        found = true;

                        GetDependencies(requiredDataSets, missingDataSets, gameDef, gds);
                        break;
                    }
                }
                if (!found)
                {
                    missingDataSets.insert(dataSetName);
                }
            }
        }

    }

    static std::vector<const GameDefinition*> GetVisibleGameDefinitions(const std::vector<GameDefinition>& gameDefinitions)
    {
        std::vector<const GameDefinition*> ret;
        ret.reserve(gameDefinitions.size());
        for (const auto& gd : gameDefinitions)
        {
            if (!gd.Hidden())
            {
                ret.emplace_back(&gd);
            }
        }
        return ret;
    }

    GameDefinition(const GameDefinition&) = default;
    GameDefinition& operator = (const GameDefinition&) = default;

    GameDefinition(IFileSystem& fileSystem, const char* gameDefinitionFile, bool isMod)
        : mIsMod(isMod)
    {
        auto stream = fileSystem.Open(gameDefinitionFile);
        assert(stream != nullptr);
        const auto jsonData = stream->LoadAllToString();
        Parse(jsonData);
    }

    GameDefinition(std::string name, std::string dataSetName, std::vector<std::string> requiredDataSets, bool isMod)
        : mName(name), mDataSetName(dataSetName), mRequiredDataSets(requiredDataSets), mIsMod(isMod)
    {

    }

    GameDefinition() = default;
    
    const std::string& Name() const { return mName; }
    const std::string& Description() const { return mDescription; }
    const std::string& Author() const { return mAuthor; }
    const std::string& InitialLevel() const { return mInitialLevel; }
    const std::string& DataSetName() const { return mDataSetName; }
    const std::vector<std::string> RequiredDataSets() const { return mRequiredDataSets; }
    bool Hidden() const { return mHidden; }
    bool IsMod() const { return mIsMod; }
private:

    void Parse(const std::string& json)
    {
        jsonxx::Object root;
        root.parse(json);
        
        mName = root.get<jsonxx::String>("Name");
        mDescription = root.get<jsonxx::String>("Description");
        mAuthor = root.get<jsonxx::String>("Author");
        mInitialLevel = root.get<jsonxx::String>("InitialLevel");
        mDataSetName = root.get<jsonxx::String>("DatasetName");
        if (root.has<jsonxx::Boolean>("Hidden"))
        {
            mHidden = root.get<jsonxx::Boolean>("Hidden");
        }

        if (root.has<jsonxx::Array>("RequiredDatasets"))
        {
            const auto requiredSets = root.get<jsonxx::Array>("RequiredDatasets");
            mRequiredDataSets.reserve(requiredSets.size());
            for (size_t i = 0; i < requiredSets.size(); i++)
            {
                mRequiredDataSets.emplace_back(requiredSets.get<jsonxx::String>(static_cast<Uint32>(i)));
            }
        }
    }

    std::string mName;
    std::string mDescription;
    std::string mAuthor;
    std::string mInitialLevel;
    std::string mDataSetName; // Name of this data set
    bool mHidden = false;
    std::vector<std::string> mRequiredDataSets;
    bool mIsMod = false;
};

class ResourceMapper
{
public:
    ResourceMapper() = default;

    ResourceMapper(ResourceMapper&& rhs)
    {
        *this = std::move(rhs);
    }

    ResourceMapper& operator = (ResourceMapper&& rhs)
    {
        if (this != &rhs)
        {
            mAnimMaps = std::move(rhs.mAnimMaps);
        }
        return *this;
    }

    ResourceMapper(IFileSystem& fileSystem, const char* resourceMapFile)
    {
        auto stream = fileSystem.Open(resourceMapFile);
        assert(stream != nullptr);
        const auto jsonData = stream->LoadAllToString();
        Parse(jsonData);
    }

    struct AnimMapping
    {
        std::string mFile;
        Uint32 mId;
        Uint32 mBlendingMode;
    };

    const AnimMapping* Find(const char* resourceName) const
    {
        auto it = mAnimMaps.find(resourceName);
        if (it != std::end(mAnimMaps))
        {
            return &it->second;
        }
        return nullptr;
    }

    void AddAnimMapping(const std::string& resourceName, const AnimMapping& mapping)
    {
        mAnimMaps[resourceName] = mapping;
    }
private:

    std::map<std::string, AnimMapping> mAnimMaps;

    void Parse(const std::string& json)
    {
        jsonxx::Array root;
        if (!root.parse(json))
        {
            throw std::runtime_error("Can't parse resource map json");
        }

        for (size_t rootObjIndex = 0; rootObjIndex < root.size(); rootObjIndex++)
        {
            jsonxx::Object obj = root.get<jsonxx::Object>(rootObjIndex);
            if (obj.has<jsonxx::Array>("anims"))
            {
                ParseAnimResourceJson(obj);
            }
        }
    }

    void ParseAnimResourceJson(const jsonxx::Object& obj)
    {
        const jsonxx::Array& anims = obj.get<jsonxx::Array>("anims");

        const auto& file = obj.get<jsonxx::String>("file");
        const auto id = static_cast<Uint32>(obj.get<jsonxx::Number>("id"));

        for (size_t i = 0; i < anims.size(); i++)
        {
            AnimMapping mapping;
            mapping.mFile = file;
            mapping.mId = static_cast<Uint32>(id);

            const jsonxx::Object& animRecord = anims.get<jsonxx::Object>(static_cast<Uint32>(i));

            const auto& name = animRecord.get<jsonxx::String>("name");
            const auto blendMode = animRecord.get<jsonxx::Number>("blend_mode");
            mapping.mBlendingMode = static_cast<Uint32>(blendMode);

            AddAnimMapping(name, mapping);
        }
    }

};

class ResourceBase
{
public:
    virtual ~ResourceBase() = default;
    virtual void Reload() = 0;
};

class Animation : public ResourceBase
{
public:
    Animation(std::unique_ptr<Oddlib::IStream> stream)
    {
        // TODO
        std::ignore = stream;
    }

    virtual void Reload() override
    {
        // TODO
    }
};

// TODO: Handle resources that have been loaded via explicit dataset name
class ResourceCache
{
public:
    void Add(size_t resourceHash, std::shared_ptr<ResourceBase> resource)
    {
        mCache[resourceHash] = resource;
    }

    void Remove(size_t resourceHash)
    {
        auto it = mCache.find(resourceHash);
        if (it != std::end(mCache))
        {
            auto sPtr = it->second.lock();
            if (!sPtr)
            {
                mCache.erase(it);
            }
        }
    }

    template<class T>
    std::shared_ptr<T> Find(size_t resourceHash)
    {
        auto it = mCache.find(resourceHash);
        if (it != std::end(mCache))
        {
            auto sPtr = it->second.lock();
            if (!sPtr)
            {
                mCache.erase(it);
                return nullptr;
            }
            return std::static_pointer_cast<T>(sPtr);
        }
        return nullptr;
    }
private:
    std::unordered_map<size_t, std::weak_ptr<ResourceBase>> mCache;
};

template<class T>
class Resource
{
public:
    Resource(ResourceCache& cache, std::shared_ptr<T> ptr, size_t resourceNameHash)
        : mResourceNameHash(resourceNameHash), mCache(cache)
    {
        mPtr = ptr;
    }

    Resource(const Resource& rhs)
        : mCache(rhs.mCache)
    {
        *this = rhs;
    }

    Resource& operator = (const Resource& rhs)
    {
        if (this != &rhs)
        {
            mResourceNameHash = rhs.mResourceNameHash;
            mPtr = rhs.mPtr;
        }
        return *this;
    }

    Resource(size_t resourceNameHash, ResourceCache& cache, std::unique_ptr<Oddlib::IStream> stream)
        : mResourceNameHash(resourceNameHash), mCache(cache)
    {
        mPtr = std::make_shared<T>(std::move(stream));
        mCache.Add(mResourceNameHash, mPtr);
    }

    ~Resource()
    {
        mCache.Remove(mResourceNameHash);
    }

    void Reload()
    {
        mPtr->Reload();
    }

    T* Ptr()
    {
        return mPtr.get();
    }

private:
    std::shared_ptr<T> mPtr;
    size_t mResourceNameHash;
    ResourceCache& mCache;
};


class ResourceLocator
{
public:
    ResourceLocator(const ResourceLocator&) = delete;
    ResourceLocator& operator =(const ResourceLocator&) = delete;


    ResourceLocator(ResourceMapper&& resourceMapper, DataPaths&& dataPaths)
        : mResMapper(std::move(resourceMapper)), mDataPaths(std::move(dataPaths))
    {

    }

    // TOOD: Provide limited interface to this?
    DataPaths& GetDataPaths()
    {
        return mDataPaths;
    }

    void AddDataPath(const char* /*dataPath*/, Sint32 /*priority*/, const std::string& /*dataSetId*/)
    {
       // mDataPaths.Add(dataPath, priority, dataSetId);
    }

    template<typename T>
    Resource<T> Locate(const char* resourceName)
    {
        const size_t resNameHash = StringHash(resourceName);

        // Check if the resource is cached
        std::shared_ptr<T> cachedRes = mResourceCache.Find<T>(resNameHash);
        if (cachedRes)
        {
            return Resource<T>(mResourceCache, cachedRes, resNameHash);
        }

        // For each data set attempt to find resourceName by mapping
        // to a LVL/file/chunk. Or in the case of a mod dataset something else.
        const ResourceMapper::AnimMapping* animMapping = mResMapper.Find(resourceName);
        if (animMapping)
        {
            //const auto& lvlFileToFind = animMapping->mFile;
            /*
            auto stream = mDataPaths.Open(lvlFileToFind);
            if (stream)
            {
                return Resource<T>(resNameHash, mResourceCache, std::move(stream));
            }
            */
        }

        // TODO
        return Resource<T>(StringHash(""), mResourceCache, nullptr);
    }

    // This method should be used for debugging only - i.e so we can compare what resource X looks like
    // in dataset A and B.
    template<typename T>
    Resource<T> Locate(const char* resourceName, const std::string& dataSetName)
    {
        const std::string uniqueName = std::string(resourceName) + dataSetName;
        const size_t resNameHash = StringHash(uniqueName);

        // Check if the resource is cached
        std::shared_ptr<T> cachedRes = mResourceCache.Find<T>(resNameHash);
        if (cachedRes)
        {
            return Resource<T>(mResourceCache, cachedRes, resNameHash);
        }

        const ResourceMapper::AnimMapping* animMapping = mResMapper.Find(resourceName);
        if (animMapping)
        {
            // TODO: Find resource in specific data set 
            //const auto& lvlFileToFind = animMapping->mFile;
            /*
            auto stream = mDataPaths.Open(lvlFileToFind, dataSetName);
            if (stream)
            {
                return Resource<T>(resNameHash, mResourceCache, std::move(stream));
            }*/
        }

        // TODO
        return Resource<T>(StringHash(""), mResourceCache, nullptr);
    }
private:
    ResourceMapper mResMapper;
    DataPaths mDataPaths;
    ResourceCache mResourceCache;
};
