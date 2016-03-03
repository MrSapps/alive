#include <gmock/gmock.h>
#include <set>
#include "oddlib/stream.hpp"
#include <jsonxx/jsonxx.h>
#include "logger.hpp"

using namespace ::testing;

std::vector<Uint8> StringToVector(const std::string& str)
{
    return std::vector<Uint8>(str.begin(), str.end());
}

class IFileSystem
{
public:
    virtual ~IFileSystem() = default;
    virtual std::unique_ptr<Oddlib::IStream> Open(const char* fileName) = 0;

    //virtual std::string UserSettingsDirectory() = 0;

};

class FileSystem : public IFileSystem
{
public:
    virtual std::unique_ptr<Oddlib::IStream> Open(const char* fileName) override
    {
        std::ignore = fileName;
        return nullptr;
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

// AOPC, AOPSX, FoosMod etc
class GameDefinition
{
public:
    GameDefinition(IFileSystem& fileSystem, const char* gameDefinitionFile)
    {
        std::ignore = fileSystem;
        std::ignore = gameDefinitionFile;
    }

    GameDefinition() = default;

    std::string mName;
    std::string mDescription;
    std::string mAuthor;
    // TODO: initial level, how maps connect, etc.
    // Depends on DataSets
};

class ResourceMapper
{
public:
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

                mAnimMaps[name] = mapping;
            }
        }
    }
};

class ResourceBase
{
public:
};

// TODO: Should store the dataset somewhere too so explicly picking another data set 
// still loads the resource
class ResourceCache
{
public:
    void Add(ResourceBase* )
    {
        // TODO
    }

    void Remove(ResourceBase*)
    {
        // TODO
    }

    ResourceBase* Find(const char* resourceName)
    {
        auto it = mCache.find(resourceName);
        if (it != std::end(mCache))
        {
            return it->second;
        }
        return nullptr;
    }
private:
    std::map<const char*, ResourceBase*> mCache;
};

template<class T>
class Resource : public ResourceBase
{
public:
    Resource(const Resource& rhs)
        : mCache(rhs.mCache)
    {
        *this = rhs;
    }

    Resource& operator = (const Resource& rhs)
    {
        if (this != &rhs)
        {
            // TODO
        }
        return *this;
    }

    Resource(ResourceCache& cache, std::unique_ptr<Oddlib::IStream>)
        : mCache(cache)
    {
        mCache.Add(this);
    }

    ~Resource()
    {
        mCache.Remove(this);
    }

    void Reload()
    {

    }
private:
    ResourceCache& mCache;
};

class Animation
{
public:

};

using TAnimationResource = Resource < Animation >;

class ResourceLocator
{
public:
    ResourceLocator(const ResourceLocator&) = delete;
    ResourceLocator& operator =(const ResourceLocator&) = delete;

    ResourceLocator(IFileSystem& fileSystem, GameDefinition& game, const char* resourceMapFile)
        : mFs(fileSystem), mResMapper(fileSystem, resourceMapFile)
    {
        std::ignore = game;
    }

    void AddDataPath(const char* dataPath, Sint32 priority)
    {
        mDataPaths.insert({ dataPath, priority });
    }

    /*
    void AddAnimationMapping(const char* resourceName, const char* dataSetName, int id, int animationIndex, int blendingMode)
    {
        std::ignore = resourceName;
        std::ignore = dataSetName;
        std::ignore = id;
        std::ignore = animationIndex;
        std::ignore = blendingMode;
    }
    */

    template<typename T>
    Resource<T> Locate(const char* resourceName)
    {
        // Check if the resource is cached
        ResourceBase* cachedRes = mResourceCache.Find(resourceName);
        if (cachedRes)
        {
            return *static_cast<Resource<T>*>(cachedRes);
        }

        // For each data set attempt to find resourceName by mapping
        // to a LVL/file/chunk. Or in the case of a mod dataset something else.
        const ResourceMapper::AnimMapping* animMapping = mResMapper.Find(resourceName);
        if (animMapping)
        {
            for (auto& path : mDataPaths)
            {
                const auto& lvlFileToFind = animMapping->mFile;

                // TODO: We need to search in each LVL that the animMapping->mFile could be in
                // within each path.mPath, however if the data path is a path to a mod then apply
                // the override rules such as looking for PNGs instead.

                auto stream = mFs.Open((path.mPath + "\\" + lvlFileToFind).c_str());
                if (stream)
                {
                    return Resource<T>(mResourceCache, std::move(stream));
                }
            }
        }

        // TODO
        return Resource<T>(mResourceCache, nullptr);
    }

    template<typename T>
    Resource<T> Locate(const char* resourceName, const char* dataSetName)
    {
        std::ignore = resourceName;
        std::ignore = dataSetName;
        // TODO
        return Resource<T>(mResourceCache,nullptr);
    }
private:
    IFileSystem& mFs;
    ResourceMapper mResMapper;
    struct DataPath
    {
        std::string mPath;
        Sint32 mPriority;

        bool operator < (const DataPath& rhs) const
        {
            return mPriority < rhs.mPriority;
        }
    };
    std::set<DataPath> mDataPaths;
    ResourceCache mResourceCache;
};

TEST(ResourceLocator, ParseResourceMap)
{
    // TODO
}

TEST(ResourceLocator, Locate)
{
    GameDefinition aePc;
    aePc.mAuthor = "Oddworld Inhabitants";
    aePc.mDescription = "The original PC version of Oddworld Abe's Exoddus";
    aePc.mName = "Oddworld Abe's Exoddus PC";

    MockFileSystem fs;
    const std::string resourceMapsJson = R"({"anims":[{"blend_mode":1,"name":"SLIGZ.BND_417_1"},{"blend_mode":1,"name":"SLIGZ.BND_417_2"}],"file":"SLIGZ.BND","id":417})";
    EXPECT_CALL(fs, OpenProxy(StrEq("resource_maps.json"))).WillRepeatedly(Return(new Oddlib::Stream(StringToVector(resourceMapsJson))));
    EXPECT_CALL(fs, OpenProxy(StrEq("C:\\dataset_location1\\SLIGZ.BND"))).WillRepeatedly(Return(nullptr));
    EXPECT_CALL(fs, OpenProxy(StrEq("C:\\dataset_location2\\SLIGZ.BND")))
        .WillRepeatedly(Return(new Oddlib::Stream(StringToVector("test"))))
        .RetiresOnSaturation();

    // TODO: Find a way to make gmock stop caching the argument causing us to double delete
    EXPECT_CALL(fs, OpenProxy(StrEq("C:\\dataset_location2\\SLIGZ.BND")))
        .WillRepeatedly(Return(new Oddlib::Stream(StringToVector("test"))))
        .RetiresOnSaturation();

    ResourceLocator locator(fs, aePc, "resource_maps.json");

    locator.AddDataPath("C:\\dataset_location2", 2);
    locator.AddDataPath("C:\\dataset_location1", 1);
  
    // TODO: Test parsing the resource map on its own, use this to add the stuff we want to test
    //locator.AddAnimationMapping("AbeWalkLeft", "ABEBSIC.BAN", 10, 1, 2);

    TAnimationResource resMapped1 = locator.Locate<Animation>("SLIGZ.BND_417_1");
    resMapped1.Reload();

    TAnimationResource resMapped2 = locator.Locate<Animation>("SLIGZ.BND_417_1");
    resMapped2.Reload();

    // Can explicitly set the dataset to obtain it from a known location
    TAnimationResource resDirect = locator.Locate<Animation>("SLIGZ.BND_417_1", "AEPCCD1");
    resDirect.Reload();
}
