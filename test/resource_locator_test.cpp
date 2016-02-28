#include <gmock/gmock.h>

class IFileSystem
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
    // TODO: File that ids this data set
    // Files in dataset
};

class BaseResource
{
public:

};

class AnimationResource : public BaseResource
{
public:

};


class ResourceLocator
{
public:
    ResourceLocator(IFileSystem& fileSystem, GameDefinition& game)
    {

    }

    void AddDataSet(DataSet& set, int priority)
    {

    }

    BaseResource* Locate(const char* resourceName)
    {
        // For each data set attempt to find resourceName by mapping
        // to a LVL/file/chunk. Or in the case of a mod dataset something else.
    }
};

class ResourceCache
{
public:

};

TEST(ResourceLocator, Locate)
{
    IFileSystem fs;
    GameDefinition aePc;
    ResourceLocator locator(fs, aePc);

    DataSet aePcCd1;
    locator.AddDataSet(aePcCd1, 1);

    // TODO: Still want raw access to use "raw" data for debugging. e.g load pc and psx version of the sprite for AbeWalkLeft, maybe demo and mod override too.
    BaseResource* res = locator.Locate("AbeWalkLeft");
}
