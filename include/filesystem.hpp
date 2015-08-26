#pragma once

#include <string>
#include <memory>
#include <queue>

namespace Oddlib
{
    class IStream;
}

class RawCdImage;

class FileSystem
{
public:
    FileSystem();
    ~FileSystem();
    bool Init();
    const std::string& BasePath() const { return mBasePath; }
    void AddResourcePath(const std::string& path, int priority);
    std::unique_ptr<Oddlib::IStream> OpenResource(const std::wstring& name);
private:
    class ResourcePathAbstraction
    {
    public:
        ResourcePathAbstraction(const std::string& path, int priority)
            : mPath(path), mPriority(priority)
        {

        }

        int Priority() const { return mPriority; }
    private:
        std::string mPath;
        int mPriority;
    };

    class RawCdImagePath : public ResourcePathAbstraction
    {
    private:
        std::unique_ptr<RawCdImage> mCdImage;
    };

    struct ResourcePathSortByPriority
    {
        bool operator()(const ResourcePathAbstraction& lhs, const ResourcePathAbstraction& rhs) const
        {
            return lhs.Priority() < rhs.Priority();
        }
    };

    void InitBasePath();
private:
    std::string mBasePath;

    //std::priority_queue<ResourcePathAbstraction, ResourcePathSortByPriority> mResourcePaths;
};
