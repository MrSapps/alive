#pragma once

#include <string>
#include <memory>
#include <vector>

namespace Oddlib
{
    class IStream;
}

class FileSystem
{
public:
    FileSystem();
    ~FileSystem();
    bool Init();
    void AddResourcePath(const std::string& path, int priority);
    bool Exists(const std::string& name) const;
    std::unique_ptr<Oddlib::IStream> Open(const std::string& name);
    bool ResourceExists(const std::string& name) const;
    std::unique_ptr<Oddlib::IStream> OpenResource(const std::string& name);
    void DebugUi();
    std::string BasePath() const { return mBasePath; }
private:
    void SortPaths();

    class IResourcePathAbstraction
    {
    public:
        IResourcePathAbstraction(const IResourcePathAbstraction&) = delete;
        IResourcePathAbstraction& operator = (const IResourcePathAbstraction&) const = delete;
        virtual ~IResourcePathAbstraction() = default;
        IResourcePathAbstraction(const std::string& path, int priority) : mPath(path), mPriority(priority) { }
        int Priority() const { return mPriority; }
        void SetPriority(int priority) { mPriority = priority; }
        const std::string& Path() const { return mPath; }
        virtual std::unique_ptr<Oddlib::IStream> Open(const std::string& fileName) = 0;
        virtual bool Exists(const std::string& fileName) const = 0;
    private:
        std::string mPath;
        int mPriority = 0;
    };

    std::unique_ptr<IResourcePathAbstraction> MakeResourcePath(std::string path, int priority);

    class Directory : public IResourcePathAbstraction
    {
    public:
        Directory(const std::string& path, int priority);
        virtual std::unique_ptr<Oddlib::IStream> Open(const std::string& fileName) override;
        virtual bool Exists(const std::string& fileName) const override;
    };

    class RawCdImagePath : public IResourcePathAbstraction
    {
    public:
        RawCdImagePath(const std::string& path, int priority);
        virtual std::unique_ptr<Oddlib::IStream> Open(const std::string& fileName) override;
        virtual bool Exists(const std::string& fileName) const override;
    private:
        std::unique_ptr<class RawCdImage> mCdImage;
    };

    void InitBasePath();
    void InitResourcePaths();
private:
    std::string mBasePath;
    std::vector<std::unique_ptr<IResourcePathAbstraction>> mResourcePaths;
};
