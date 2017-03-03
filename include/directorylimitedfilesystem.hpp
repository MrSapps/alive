#pragma once

#include "filesystem.hpp"

class DirectoryLimitedFileSystem : public IFileSystem
{
public:
    DirectoryLimitedFileSystem(DirectoryLimitedFileSystem&&) = delete;
    DirectoryLimitedFileSystem& operator = (DirectoryLimitedFileSystem&&) = delete;

    DirectoryLimitedFileSystem(IFileSystem& fs, const std::string& directory)
        : mFs(fs)
    {
        mBasePath = directory;
        NormalizePath(mBasePath);
    }

    virtual std::string FsPath() const override
    {
        return mBasePath;
    }

    virtual bool Init() override final
    {
        return true;
    }

    virtual std::unique_ptr<Oddlib::IStream> Open(const std::string& fileName) override final
    {
        return mFs.Open(LimitPath(fileName));
    }

    virtual std::unique_ptr<Oddlib::IStream> Create(const std::string& fileName) override final
    {
        return mFs.Create(LimitPath(fileName));
    }

    virtual std::vector<std::string> EnumerateFiles(const std::string& directory, const char* filter) override final
    {
        return mFs.EnumerateFiles(LimitPath(directory), filter);
    }

    virtual std::vector<std::string> EnumerateFolders(const std::string& directory) override final
    {
        return mFs.EnumerateFolders(LimitPath(directory));
    }

    virtual bool FileExists(const std::string& fileName) override final
    {
        return mFs.FileExists(LimitPath(fileName));
    }

private:
    std::string LimitPath(const std::string& path)
    {
        return mBasePath + "/" + path;
    }

    IFileSystem& mFs;
    std::string mBasePath;
};
