#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>

#include "proxy_rapidjson.hpp"
#include "oddlib/stream.hpp"
#include "filesystem.hpp"
#include "logger.hpp"

namespace JsonDeserializer
{
    namespace Detail
    {
        template<class SizeType>
        void Reserve(std::vector<std::string>& c, SizeType size)
        {
            c.reserve(size);
        }

        template<class SizeType>
        void Reserve(std::set<std::string>& /*c*/, SizeType /*size*/)
        {
            // Can't preallocate a set
        }

        template<class ElementType>
        void Add(std::vector<std::string>& c, ElementType element)
        {
            c.push_back(element);
        }

        template<class ElementType>
        void Add(std::set<std::string>& c, ElementType element)
        {
            c.insert(element);
        }
    }

    template<typename ObjectType, typename StringVector>
    void ReadStringArray(const char* jsonArrayName, const ObjectType& jsonObject, StringVector& result)
    {
        if (jsonObject.HasMember(jsonArrayName))
        {
            const auto& jsonArray = jsonObject[jsonArrayName].GetArray();
            Detail::Reserve(result, jsonArray.Size());
            for (const auto& arrayElement : jsonArray)
            {
                Detail::Add(result, arrayElement.GetString());
            }
        }
    }
}

class DataSetIdentifiers
{
public:
    DataSetIdentifiers() = default;
    DataSetIdentifiers(IFileSystem& fs, const char* dataSetsIdsFileName);
    bool IsMetaPath(const std::string& dataSetName) const;
    std::string Identify(IFileSystem& fs) const;
private:
    struct DataPathFiles
    {
        std::vector<std::string> mContainAnyOf;
        std::vector<std::string> mContainAllOf;
        std::vector<std::string> mMustNotContain;
    };

    bool DoMatchFileSystemPathWithDataSetIdentifier(IFileSystem& fs, const std::pair<std::string, struct DataPathFiles>& dataPathId) const;
    void Parse(const std::string& json);

private:
    std::map<std::string, DataPathFiles> mDataPathIds;

    // A meta path is one that exists just to include other paths
    // e.g AePsx includes AePsxCd1 and AxePsxCd1, but AePsx won't actually have a path as such
    std::set<std::string> mMetaPaths;
};


class DataPaths
{
public:
    struct Path
    {
    public:
        Path(std::string dataSetName, const class GameDefinition* sourceGameDefinition)
            : mDataSetName(dataSetName), mSourceGameDefinition(sourceGameDefinition)
        {

        }

        bool operator == (const Path& other) const
        {
            return (mDataSetName == other.mDataSetName && mSourceGameDefinition == other.mSourceGameDefinition);
        }

        std::string mDataSetName;
        std::string mDataSetPath;
        const GameDefinition* mSourceGameDefinition;
    };

    using PathVector = std::vector<Path>;

    DataPaths(const DataPaths&) = delete;
    DataPaths& operator = (const DataPaths&) = delete;

    DataPaths(DataPaths&& other)
        : mGameFs(other.mGameFs)
    {
        *this = std::move(other);
    }

    DataPaths& operator = (DataPaths&& other)
    {
        mDataSetPathsJsonFileName = std::move(other.mDataSetPathsJsonFileName);
        mIds = std::move(other.mIds);
        mPaths = std::move(other.mPaths);
        return *this;
    }

    DataPaths(IFileSystem& fs, const char* idsFileName, const char* pathsFileName)
        : mGameFs(fs), mDataSetPathsJsonFileName(pathsFileName), mIds(fs, idsFileName)
    {
        TRACE_ENTRYEXIT;
        if (mGameFs.FileExists(mDataSetPathsJsonFileName))
        {
            auto stream = mGameFs.Open(mDataSetPathsJsonFileName);
            std::vector<std::string> paths = Parse(stream->LoadAllToString());
            for (const auto& path : paths)
            {
                Add(path);
            }
        }
    }

    void Persist()
    {
        rapidjson::Document jsonDoc;
        jsonDoc.SetObject();
        rapidjson::Value pathsArray(rapidjson::kArrayType);
        rapidjson::Document::AllocatorType& allocator = jsonDoc.GetAllocator();

        for (const auto& path : mPaths)
        {
            rapidjson::Value str;
            str.SetString(path.second.c_str(), allocator);
            pathsArray.PushBack(str, allocator);
        }

        jsonDoc.AddMember("paths", pathsArray, allocator);
        rapidjson::StringBuffer strbuf;
        rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
        jsonDoc.Accept(writer);

        const char* jsonString = strbuf.GetString();
        auto stream = mGameFs.Create(mDataSetPathsJsonFileName);
        stream->Write(std::string(jsonString));
    }

    bool Add(const std::string& path, const std::string& expectedId = "")
    {
        std::string pathCopy = path;
        std::string id = Identify(pathCopy); // May change pathCopy to parent dir
        if (!id.empty())
        {
            auto it = mPaths.find(id);
            if (it == std::end(mPaths))
            {

                if (expectedId.empty() == false && id != expectedId)
                {
                    LOG_ERROR("Path " << pathCopy << " identified as " << id << " but we are looking for " << expectedId);
                    return false;
                }
                else
                {
                    LOG_INFO("Path " << pathCopy << " identified as " << id);
                    mPaths[id] = pathCopy;
                    return true;
                }
            }
            else
            {
                LOG_INFO("Path " << pathCopy
                    << " identified as " << id
                    << " but ignoring because we already have the following path "
                    << it->second
                    << " for " << id);
                return false;
            }
        }
        return false;
    }

    const std::string& PathFor(const std::string& id)
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

    std::vector<std::string> MissingDataSetPaths(const std::vector<std::string>& requiredSets)
    {
        std::vector<std::string> ret;
        for (const auto& dataset : requiredSets)
        {
            if (mIds.IsMetaPath(dataset))
            {
                continue;
            }

            if (!PathFor(dataset).empty())
            {
                continue;
            }

            ret.emplace_back(dataset);
        }
        return ret;
    }

    bool SetActiveDataPaths(IFileSystem& fs, const PathVector& paths);

    struct FileSystemInfo
    {
        FileSystemInfo(const std::string& name, bool isMod, std::unique_ptr<IFileSystem> fs)
            : mDataSetName(name), mIsMod(isMod), mFileSystem(std::move(fs))
        {

        }

        FileSystemInfo(FileSystemInfo&& rhs)
        {
            *this = std::move(rhs);
        }

        FileSystemInfo& operator = (FileSystemInfo&& rhs)
        {
            mDataSetName = std::move(rhs.mDataSetName);
            mIsMod = rhs.mIsMod;
            mFileSystem = std::move(rhs.mFileSystem);
            return *this;
        }

        std::string mDataSetName;
        bool mIsMod;
        std::unique_ptr<IFileSystem> mFileSystem;
    };
    const std::vector<FileSystemInfo>& ActiveDataPaths() const { return mActiveDataPaths; }
    IFileSystem& GameFs() const { return mGameFs; }
private:

    std::string Identify(std::string& path) const
    {
        std::string id;
        auto dataSetFs = IFileSystem::Factory(mGameFs, path);
        if (dataSetFs)
        {
            id = mIds.Identify(*dataSetFs);
        }
        if (id.empty() || !dataSetFs)
        {
            LOG_WARNING("Path " << path << " could not be identified, trying parent directory..");
            path = IFileSystem::Parent(path);
            dataSetFs = IFileSystem::Factory(mGameFs, path);
            if (dataSetFs)
            {
                id = mIds.Identify(*dataSetFs);
            }

            if (id.empty())
            {
                LOG_ERROR("Path " << path << " could not be identified");
            }
        }
        return id;
    }

    std::vector<FileSystemInfo> mActiveDataPaths;

    std::map<std::string, std::string> mPaths;

    std::vector<std::string> Parse(const std::string& json)
    {
        std::vector<std::string> paths;

        rapidjson::Document document;
        document.Parse(json.c_str());

        JsonDeserializer::ReadStringArray("paths", document, paths);

        return paths;
    }

    // The "owner" file system
    IFileSystem& mGameFs;

    std::string mDataSetPathsJsonFileName;

    // To match to what a game def wants (AePcCd1, AoDemoPsx etc)
    // we use SLUS codes for PSX or if it contains ABEWIN.EXE etc then its AoPc.
    DataSetIdentifiers mIds;
    const /*static*/ std::string mNotFoundResult;
};

class GameDefinition
{
private:
    static bool Exists(const std::string& dataSetName, const DataPaths::PathVector& dataSets)
    {
        for (const DataPaths::Path& dataSet : dataSets)
        {
            if (dataSet.mDataSetName == dataSetName)
            {
                return true;
            }
        }
        return false;
    }

    static const GameDefinition* Find(const std::string& dataSetName, const std::vector<const GameDefinition*>& gds)
    {
        for (const GameDefinition* gd : gds)
        {
            if (gd->DataSetName() == dataSetName)
            {
                return gd;
            }
        }
        return nullptr;
    }

    static void GetDependencies(int& priority, DataPaths::PathVector& requiredDataSets, std::set<std::string>& missingDataSets, const GameDefinition* gd, const std::vector<const GameDefinition*>& gds)
    {
        requiredDataSets.emplace_back(gd->DataSetName(), gd);
        for (const std::string& dataSetName : gd->RequiredDataSets())
        {
            // Skip if already processed to avoid inf recursion on cycles
            if (!Exists(dataSetName, requiredDataSets))
            {
                // Find the game def that contains the dataset that the current game def requires
                const GameDefinition* foundGameDef = Find(dataSetName, gds);
                if (foundGameDef)
                {
                    // Recursively check the this game defs dependencies too
                    GetDependencies(priority, requiredDataSets, missingDataSets, foundGameDef, gds);
                }
                else
                {
                    missingDataSets.insert(dataSetName);
                }
            }
        }
    }

public:

    // DFS graph walk
    // This means if we have:
    // mod -> [ AePsx, AePc]
    // Where AePsx depends on AePsxCd1 and AePsxCd1 then the search order of resources would be:
    // mod, AePsx, AePsxCd1, AePsxCd2, AePc.
    static void GetDependencies(DataPaths::PathVector& requiredDataSets, std::set<std::string>& missingDataSets, const GameDefinition* gd, const std::vector<const GameDefinition*>& gds)
    {
        int priority = 0;
        GetDependencies(priority, requiredDataSets, missingDataSets, gd, gds);
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
        mContainingArchive = fileSystem.FsPath();
    }

    GameDefinition(std::string name, std::string dataSetName, std::vector<std::string> requiredDataSets, bool isMod)
        : mName(name), mDataSetName(dataSetName), mRequiredDataSets(requiredDataSets), mIsMod(isMod)
    {

    }

    GameDefinition() = default;

    const std::string& Name() const { return mName; }
    const std::string& Description() const { return mDescription; }
    const std::string& Author() const { return mAuthor; }
    const std::string& GameScript() const { return mGameScript; }
    const std::string& DataSetName() const { return mDataSetName; }
    const std::vector<std::string> RequiredDataSets() const { return mRequiredDataSets; }
    bool Hidden() const { return mHidden; }
    bool IsMod() const { return mIsMod; }
    const std::string ContainingArchive() const { return mContainingArchive; }
private:

    void Parse(const std::string& json);

    std::string mName;
    std::string mDescription;
    std::string mAuthor;
    std::string mGameScript;
    std::string mDataSetName; // Name of this data set
    bool mHidden = false;
    std::vector<std::string> mRequiredDataSets;
    bool mIsMod = false;
    std::string mContainingArchive;
};
