#include <gmock/gmock.h>

class IFileSystem
{
public:

};

class FileSystem : public IFileSystem
{
public:

};

// AOPC, AOPSX, FoosMod etc
class GameDefinition
{
public:
    std::string mName;
    std::string mDescription;
    std::string mAuthor;
    // TODO: initial level, how maps connect, etc.
    // Depends on DataSets
};

class DataSet
{
public:
    DataSet(const char* dataSetPath, const char* dataSetName)
    {

    }

    // TODO: File that ids this data set
    // Files in dataset
    std::vector<std::string> mFiles;
};

template<class T>
class Resource
{
public:
    void Reload()
    {

    }
};

class Animation
{
public:

};

using TAnimationResource = Resource < Animation >;

class ResourceLocator
{
public:
    ResourceLocator(IFileSystem& fileSystem, GameDefinition& game)
    {

    }

    void AddDataSet(DataSet& set, int priority)
    {

    }

    void AddAnimationMapping(const char* resourceName, const char* dataSetName, int id, int animationIndex, int blendingMode)
    {

    }

    template<typename T>
    Resource<T>* Locate(const char* resourceName)
    {
        // For each data set attempt to find resourceName by mapping
        // to a LVL/file/chunk. Or in the case of a mod dataset something else.
    }

    template<typename T>
    Resource<T>* LocateOriginal(const char* dataSetName, const char* lvlName, const char* fileName, int id, int animationIndex, int blendMode)
    {

    }
};

TEST(ResourceLocator, Locate)
{
    GameDefinition aePc;
    aePc.mAuthor = "Oddworld Inhabitants";
    aePc.mDescription = "The original PC version of Oddworld Abe's Exoddus";
    aePc.mName = "Oddworld Abe's Exoddus PC";

    FileSystem fs;
    ResourceLocator locator(fs, aePc);

    DataSet aePcCd1("C:\\dataset_location", "AEPCCD1");
    aePcCd1.mFiles.push_back("mi.lvl\\ABEBSIC.BAN");
    aePcCd1.mFiles.push_back("INGRDNT.DDV");

    locator.AddDataSet(aePcCd1, 1);

    locator.AddAnimationMapping("AbeWalkLeft", "ABEBSIC.BAN", 10, 1, 2);

    // TODO: Still want raw access to use "raw" data for debugging. e.g load pc and psx version of the sprite for AbeWalkLeft, maybe demo and mod override too.
    // or could just create a mapper per data set (?)
    TAnimationResource* resMapped = locator.Locate<Animation>("AbeWalkLeft");
    resMapped->Reload();

    TAnimationResource* resDirect = locator.LocateOriginal<Animation>("AEPCCD1", "mi.lvl", "ABEBSIC.BAN", 10, 1, 2);
}
