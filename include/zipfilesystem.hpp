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
    virtual std::unique_ptr<Oddlib::IStream> Create(const std::string& fileName) override;
    virtual std::vector<std::string> EnumerateFiles(const std::string& directory, const char* filter) override;
    virtual std::vector<std::string> EnumerateFolders(const std::string& directory) override;
    virtual bool FileExists(const std::string& fileName) override;
    virtual std::string FsPath() const override;

private:
    const u32 kEndOfCentralDirectory = 0x06054b50;
    const u32 kEndOfCentralDirectoryRecordSizeWithMagic = 22;
    struct EndOfCentralDirectoryRecord
    {
        u16 mThisDiskNumber = 0;
        u16 mStartCentralDirectoryDiskNumber = 0;
        u16 mNumEntriesInCentaralDirectoryOnThisDisk = 0;
        u16 mNumEntriesInCentaralDirectory = 0;
        u32 mCentralDirectorySize = 0;
        u32 mCentralDirectoryStartOffset = 0;
        u16 mCommentSize = 0;
        // [comment data]

        void DeSerialize(Oddlib::IStream& stream)
        {
            stream.Read(mThisDiskNumber);
            stream.Read(mStartCentralDirectoryDiskNumber);
            stream.Read(mNumEntriesInCentaralDirectoryOnThisDisk);
            stream.Read(mNumEntriesInCentaralDirectory);
            stream.Read(mCentralDirectorySize);
            stream.Read(mCentralDirectoryStartOffset);
            stream.Read(mCommentSize);
        }
    };

    EndOfCentralDirectoryRecord mEndOfCentralDirectoryRecord;

    bool LocateEndOfCentralDirectoryRecord();
    bool LoadCentralDirectoryRecords();

    std::unique_ptr<Oddlib::IStream> mStream;
    std::string mFileName;


    // DataDescriptor signature = 0x08074b50
    struct DataDescriptor
    {
        u32 mCrc32;
        u32 mCompressedSize;
        u32 mUnCompressedSize;

        void DeSerialize(Oddlib::IStream& stream)
        {
            stream.Read(mCrc32);
            stream.Read(mCompressedSize);
            stream.Read(mUnCompressedSize);
        }
    };

    const u32 kLocalFileHeader = 0x04034b50;
    struct LocalFileHeader
    {
        u16 mMinVersionRequiredToExtract;
        u16 mGeneralPurposeFlags;
        u16 mCompressionMethod;
        u16 mFileLastModifiedTime;
        u16 mFileLastModifiedDate;
        DataDescriptor mDataDescriptor;
        u16 mFileNameLength;
        u16 mExtraFieldLength;
        std::string mFileName;

        // extra field

        void DeSerialize(Oddlib::IStream& stream)
        {
            stream.Read(mMinVersionRequiredToExtract);
            stream.Read(mGeneralPurposeFlags);
            stream.Read(mCompressionMethod);
            stream.Read(mFileLastModifiedTime);
            stream.Read(mFileLastModifiedDate);
            mDataDescriptor.DeSerialize(stream);
            stream.Read(mFileNameLength);
            stream.Read(mExtraFieldLength);
        }
    };

    const u32 kCentralDirectory = 0x02014b50;
    struct CentralDirectoryRecord
    {
        u16 mCreatedByVersion;
        LocalFileHeader mLocalFileHeader;
        u16 mFileCommentLength;
        u16 mFileDiskNumber; // Where file starts
        u16 mInternalFileAttributes;
        u32 mExternalFileAttributes;
        u32 mRelativeLocalFileHeaderOffset;
        // file name
        // extra field
        // file comment

        void DeSerialize(Oddlib::IStream& stream)
        {
            stream.Read(mCreatedByVersion);
            mLocalFileHeader.DeSerialize(stream);
            stream.Read(mFileCommentLength);
            stream.Read(mFileDiskNumber);
            stream.Read(mInternalFileAttributes);
            stream.Read(mExternalFileAttributes);
            stream.Read(mRelativeLocalFileHeaderOffset);

            if (mLocalFileHeader.mFileNameLength > 0)
            {
                mLocalFileHeader.mFileName.resize(mLocalFileHeader.mFileNameLength);
                stream.Read(mLocalFileHeader.mFileName);
            }

            const u32 sizeOfExtraFieldAndFileComment = mLocalFileHeader.mExtraFieldLength + mFileCommentLength;
            if (sizeOfExtraFieldAndFileComment > 0)
            {
                stream.Seek(stream.Pos() + sizeOfExtraFieldAndFileComment);
            }
        }
    };


    std::vector<CentralDirectoryRecord> mRecords;
};
