#pragma once

#include "resourcemapper.hpp"

// Actually "ZIP64" file system, which removes 65k file limit and 4GB zip file size limit
class ZipFileSystem : public IFileSystem
{
public:
    ZipFileSystem(const std::string& zipFile, IFileSystem& fs);

    virtual bool Init() override;
    virtual std::unique_ptr<Oddlib::IStream> Open(const std::string& fileName) override;
    virtual std::vector<std::string> EnumerateFiles(const std::string& directory, const char* filter) override;
    virtual bool FileExists(const std::string& fileName) override;
    virtual std::string FsPath() const override;

private:
    void LocateEndOfCentralDirectoryRecord();

    std::unique_ptr<Oddlib::IStream> mStream;
    std::string mFileName;
};
