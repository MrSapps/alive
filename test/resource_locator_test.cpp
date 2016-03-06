#include <gmock/gmock.h>
#include <set>
#include "oddlib/stream.hpp"
#include <jsonxx/jsonxx.h>
#include "logger.hpp"

using namespace ::testing;

size_t StringHash(const char* s)
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

size_t StringHash(const std::string& s)
{
    return StringHash(s.c_str());
}

std::vector<Uint8> StringToVector(const std::string& str)
{
    return std::vector<Uint8>(str.begin(), str.end());
}

class IFileSystem
{
public:
    virtual ~IFileSystem() = default;
    virtual std::unique_ptr<Oddlib::IStream> Open(const char* fileName) = 0;

    // TODO impl this and other required helpers
    //virtual std::string UserSettingsDirectory() = 0;
};

class FileSystem : public IFileSystem
{
public:
    virtual std::unique_ptr<Oddlib::IStream> Open(const char* fileName) override
    {
        return std::make_unique<Oddlib::Stream>(fileName);
    }
};

class MockFileSystem : public IFileSystem
{
public:
    virtual std::unique_ptr<Oddlib::IStream> Open(const char* fileName) override
    {
        // Can't mock unique_ptr return, so mock raw one which the unique_ptr one will call
        return std::unique_ptr<Oddlib::IStream>(OpenProxy(fileName));
    }

    MOCK_METHOD1(OpenProxy, Oddlib::IStream*(const char*));
};

enum class DataTypes
{
    eAoPc,
    eAoPcDemo,
    eAoPsx,
    eAoPsxDemo,
    eAePc,
    eAePcDemo,
    eAePsxCd1,
    eAePsxCd2,
    eAePsxDemo
};

const char* ToString(DataTypes type)
{
    switch (type)
    {
    case DataTypes::eAoPc:      return "eAoPc";
    case DataTypes::eAoPcDemo:  return "eAoPcDemo";
    case DataTypes::eAoPsx:     return "eAoPsx";
    case DataTypes::eAoPsxDemo: return "eAoPsxDemo";
    case DataTypes::eAePc:      return "eAePc";
    case DataTypes::eAePcDemo:  return "eAePcDemo";
    case DataTypes::eAePsxCd1:  return "eAePsxCd1";
    case DataTypes::eAePsxCd2:  return "eAePsxCd2";
    case DataTypes::eAePsxDemo: return "eAePsxDemo";
    default: abort();
    }
}

// AOPC, AOPSX, FoosMod etc
class GameDefinition
{
public:
    GameDefinition(IFileSystem& fileSystem, const char* gameDefinitionFile)
    {
        // TODO load from json file
        std::ignore = fileSystem;
        std::ignore = gameDefinitionFile;
    }

    GameDefinition() = default;

private:
    std::string mName;
    std::string mDescription;
    std::string mAuthor;
    std::string mInitialLevel;
    std::vector<DataTypes> mRequiredDataSets;
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
        jsonxx::Object root;
        root.parse(json);
        if (root.has<jsonxx::Array>("anims"))
        {
            const jsonxx::Array& anims = root.get<jsonxx::Array>("anims");

            const auto& file = root.get<jsonxx::String>("file");
            const auto id = static_cast<Uint32>(root.get<jsonxx::Number>("id"));

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
    std::map<size_t, std::weak_ptr<ResourceBase>> mCache;
};


template<class T>
class Resource
{
public:
    Resource(ResourceCache& cache, std::shared_ptr<T> ptr)
        : mCache(cache)
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

class DataPaths
{
public:
    DataPaths(const DataPaths&) = delete;
    DataPaths& operator = (const DataPaths&) = delete;

    DataPaths(IFileSystem& fs)
        :mFs(fs)
    {

    }

    std::unique_ptr<Oddlib::IStream> Open(const std::string& fileName)
    {
        for (auto& path : mDataPaths)
        {
            // TODO: We need to search in each LVL that the animMapping->mFile could be in
            // within each path.mPath, however if the data path is a path to a mod then apply
            // the override rules such as looking for PNGs instead.
            auto stream = mFs.Open((path.mPath + "\\" + fileName).c_str());
            if (stream)
            {
                return stream;
            }
        }
        return nullptr;
    }

    void Add(const char* dataPath, Sint32 priority)
    {
        mDataPaths.insert({ dataPath, priority });
    }
private:
    struct DataPath
    {
        std::string mPath;
        Sint32 mPriority;

        // Sort such that the lowest priority number is first.
        bool operator < (const DataPath& rhs) const
        {
            return mPriority < rhs.mPriority;
        }
    };

    std::set<DataPath> mDataPaths;
    IFileSystem& mFs;
};

class ResourceLocator
{
public:
    ResourceLocator(const ResourceLocator&) = delete;
    ResourceLocator& operator =(const ResourceLocator&) = delete;

    ResourceLocator(IFileSystem& fileSystem, GameDefinition& game, ResourceMapper&& resourceMapper)
        : mDataPaths(fileSystem), mResMapper(std::move(resourceMapper))
    {
        std::ignore = game;
    }

    void AddDataPath(const char* dataPath, Sint32 priority)
    {
        mDataPaths.Add(dataPath, priority);
    }

    template<typename T>
    Resource<T> Locate(const char* resourceName)
    {
        const size_t resNameHash = StringHash(resourceName);

        // Check if the resource is cached
        std::shared_ptr<T> cachedRes = mResourceCache.Find<T>(resNameHash);
        if (cachedRes)
        {
            return Resource<T>(mResourceCache, cachedRes);
        }

        // For each data set attempt to find resourceName by mapping
        // to a LVL/file/chunk. Or in the case of a mod dataset something else.
        const ResourceMapper::AnimMapping* animMapping = mResMapper.Find(resourceName);
        if (animMapping)
        {
            const auto& lvlFileToFind = animMapping->mFile;
            auto stream = mDataPaths.Open(lvlFileToFind);
            if (stream)
            {
                return Resource<T>(resNameHash, mResourceCache, std::move(stream));
            }
        }

        // TODO
        return Resource<T>(StringHash(""), mResourceCache, nullptr);
    }

    // This method should be used for debugging only - i.e so we can compare what resource X looks like
    // in dataset A and B.
    template<typename T>
    Resource<T> Locate(const char* resourceName, DataTypes dataSetName)
    {
        const std::string uniqueName = std::string(resourceName) + ToString(dataSetName);
        const size_t resNameHash = StringHash(uniqueName);

        // Check if the resource is cached
        std::shared_ptr<T> cachedRes = mResourceCache.Find<T>(resNameHash);
        if (cachedRes)
        {
            return Resource<T>(mResourceCache, cachedRes);
        }

        const ResourceMapper::AnimMapping* animMapping = mResMapper.Find(resourceName);
        if (animMapping)
        {
            // TODO: Find resource in specific data set 
            //const auto& lvlFileToFind = animMapping->mFile;
            //auto stream = mDataPaths.Open(lvlFileToFind, dataSetName);
        }

        // TODO
        return Resource<T>(StringHash(""), mResourceCache, nullptr);
    }
private:
    DataPaths mDataPaths;
    ResourceMapper mResMapper;
    ResourceCache mResourceCache;
};

TEST(ResourceLocator, DataPathsOpen)
{
    MockFileSystem fs;

    DataPaths paths(fs);

    paths.Add("C:\\dataset_location1", 1);
    paths.Add("C:\\dataset_location2", 2);

    EXPECT_CALL(fs, OpenProxy(StrEq("C:\\dataset_location1\\SLIGZ.BND")))
        .WillRepeatedly(Return(nullptr));

    EXPECT_CALL(fs, OpenProxy(StrEq("C:\\dataset_location2\\SLIGZ.BND")))
        .Times(1)
        .WillOnce(Return(new Oddlib::Stream(StringToVector("test"))));

    auto stream = paths.Open("SLIGZ.BND");
    ASSERT_NE(nullptr, stream);

    const auto str = stream->LoadAllToString();
    ASSERT_EQ(str, "test");
}

TEST(ResourceLocator, Cache)
{
    ResourceCache cache;

    const size_t resNameHash = StringHash("foo");
    ASSERT_EQ(nullptr, cache.Find<Animation>(resNameHash));
    {
        Resource<Animation> res1(resNameHash, cache, nullptr);

        std::shared_ptr<Animation> cached = cache.Find<Animation>(resNameHash);
        ASSERT_NE(nullptr, cached);
        ASSERT_EQ(cached.get(), res1.Ptr());
    }
    ASSERT_EQ(nullptr, cache.Find<Animation>(resNameHash));
}

TEST(ResourceLocator, ParseResourceMap)
{
    const std::string resourceMapsJson = R"({"anims":[{"blend_mode":1,"name":"SLIGZ.BND_417_1"},{"blend_mode":1,"name":"SLIGZ.BND_417_2"}],"file":"SLIGZ.BND","id":417})";

    MockFileSystem fs;

    EXPECT_CALL(fs, OpenProxy(StrEq("resource_maps.json")))
        .WillRepeatedly(Return(new Oddlib::Stream(StringToVector(resourceMapsJson))));

    ResourceMapper mapper(fs, "resource_maps.json");
    
    const ResourceMapper::AnimMapping* r0 = mapper.Find("I don't exist");
    ASSERT_EQ(nullptr, r0);

    const ResourceMapper::AnimMapping* r1 = mapper.Find("SLIGZ.BND_417_1");
    ASSERT_NE(nullptr, r1);
    ASSERT_EQ("SLIGZ.BND", r1->mFile);
    ASSERT_EQ(417u, r1->mId);
    ASSERT_EQ(1u, r1->mBlendingMode);

    const ResourceMapper::AnimMapping* r2 = mapper.Find("SLIGZ.BND_417_2");
    ASSERT_NE(nullptr, r2);
    ASSERT_EQ("SLIGZ.BND", r2->mFile);
    ASSERT_EQ(417u, r2->mId);
    ASSERT_EQ(1u, r2->mBlendingMode);

}

TEST(ResourceLocator, ParseGameDefinition)
{
    // TODO
    MockFileSystem fs;
    GameDefinition gd(fs, "test_game_definition.json");
}

TEST(ResourceLocator, LocateAnimation)
{
    GameDefinition aePc;
    /*
    aePc.mAuthor = "Oddworld Inhabitants";
    aePc.mDescription = "The original PC version of Oddworld Abe's Exoddus";
    aePc.mName = "Oddworld Abe's Exoddus PC";
    */

    MockFileSystem fs;

    EXPECT_CALL(fs, OpenProxy(StrEq("C:\\dataset_location1\\SLIGZ.BND")))
        .WillRepeatedly(Return(nullptr));

    EXPECT_CALL(fs, OpenProxy(StrEq("C:\\dataset_location2\\SLIGZ.BND")))
        .Times(1)
        .WillOnce(Return(new Oddlib::Stream(StringToVector("test"))));   // For SLIGZ.BND_417_1, 2nd call should be cached

    ResourceMapper mapper;
    mapper.AddAnimMapping("SLIGZ.BND_417_1", { "SLIGZ.BND", 417, 1 });
    mapper.AddAnimMapping("SLIGZ.BND_417_2", { "SLIGZ.BND", 417, 1 });

    ResourceLocator locator(fs, aePc, std::move(mapper));

    locator.AddDataPath("C:\\dataset_location2", 2);
    locator.AddDataPath("C:\\dataset_location1", 1);
  
    Resource<Animation> resMapped1 = locator.Locate<Animation>("SLIGZ.BND_417_1");
    resMapped1.Reload();

    Resource<Animation> resMapped2 = locator.Locate<Animation>("SLIGZ.BND_417_1");
    resMapped2.Reload();

    // Can explicitly set the dataset to obtain it from a known location
    Resource<Animation> resDirect = locator.Locate<Animation>("SLIGZ.BND_417_1", DataTypes::eAePsxCd1);
    resDirect.Reload();
}

TEST(ResourceLocator, LocateFmv)
{
    // TODO
}

TEST(ResourceLocator, LocateSound)
{
    // TODO
}

TEST(ResourceLocator, LocateMusic)
{
    // TODO
}

TEST(ResourceLocator, LocateCamera)
{
    // TODO
}

TEST(ResourceLocator, LocatePath)
{
    // TODO
}
