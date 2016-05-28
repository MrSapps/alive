#pragma once

#include "resourcemapper.hpp"

// Actually "ZIP64" file system, which removes 65k file limit and 4GB zip file size limit
// TODO: Add ZIP64 extensions, currently only supports "ZIP32" which is enough for now
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
    const Uint32 kEndOfCentralDirectory = 0x06054b50;
    struct EndOfCentralDirectoryRecord
    {
        Uint16 mThisDiskNumber = 0;
        Uint16 mStartCentralDirectoryDiskNumber = 0;
        Uint16 mNumEntriesInCentaralDirectoryOnThisDisk = 0;
        Uint16 mNumEntriesInCentaralDirectory = 0;
        Uint32 mCentralDirectorySize = 0;
        Uint32 mCentralDirectoryStartOffset = 0;
        Uint16 mCommentSize = 0;
        // [comment data]

        void DeSerialize(Oddlib::IStream& stream)
        {
            stream.ReadUInt16(mThisDiskNumber);
            stream.ReadUInt16(mStartCentralDirectoryDiskNumber);
            stream.ReadUInt16(mNumEntriesInCentaralDirectoryOnThisDisk);
            stream.ReadUInt16(mNumEntriesInCentaralDirectory);
            stream.ReadUInt32(mCentralDirectorySize);
            stream.ReadUInt32(mCentralDirectoryStartOffset);
            stream.ReadUInt16(mCommentSize);
        }
    };
    EndOfCentralDirectoryRecord mEndOfCentralDirectoryRecord;

    bool LocateEndOfCentralDirectoryRecord();

    std::unique_ptr<Oddlib::IStream> mStream;
    std::string mFileName;
};
