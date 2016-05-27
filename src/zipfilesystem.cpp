#pragma once

#include <SDL_stdinc.h>
#include <memory>
#include "zipfilesystem.hpp"
#include "unzip.h"
#include "ioapi.h"
#include "iowin32.h"

#undef max


// DataDescriptor signature = 0x08074b50
struct DataDescriptor
{
    Uint32 mCrc32;
    Uint32 mCompressedSize;
    Uint32 mUnCompressedSize;

    void DeSerialize(Oddlib::IStream& stream)
    {
        stream.ReadUInt32(mCrc32);
        stream.ReadUInt32(mCompressedSize);
        stream.ReadUInt32(mUnCompressedSize);
    }
};

// Local file header signature = 0x04034b50
struct LocalFileHeader
{
    Uint16 mMinVersionRequiredToExtract;
    Uint16 mGeneralPurposeFlags;
    Uint16 mCompressionMethod;
    Uint16 mFileLastModifiedTime;
    Uint16 mFileLastModifiedDate;
    DataDescriptor mDataDescriptor;
    Uint16 mFileNameLength;
    Uint16 mExtraFieldLength;
    std::string mFileName;

    // extra field

    void DeSerialize(Oddlib::IStream& stream)
    {
        stream.ReadUInt16(mMinVersionRequiredToExtract);
        stream.ReadUInt16(mGeneralPurposeFlags);
        stream.ReadUInt16(mCompressionMethod);
        stream.ReadUInt16(mFileLastModifiedTime);
        stream.ReadUInt16(mFileLastModifiedDate);
        mDataDescriptor.DeSerialize(stream);
        stream.ReadUInt16(mFileNameLength);
        stream.ReadUInt16(mExtraFieldLength);
    }
};

const Uint32 kCentralDirectory = 0x02014b50;
struct CentralDirectoryRecord
{
    Uint16 mCreatedByVersion;
    LocalFileHeader mLocalFileHeader;
    Uint16 mFileCommentLength;
    Uint16 mFileDiskNumber; // Where file starts
    Uint16 mInternalFileAttributes;
    Uint32 mExternalFileAttributes;
    Uint32 mRelativeLocalFileHeaderOffset;
    // file name
    // extra field
    // file comment

    void DeSerialize(Oddlib::IStream& stream)
    {
        stream.ReadUInt16(mCreatedByVersion);
        mLocalFileHeader.DeSerialize(stream);
        stream.ReadUInt16(mFileCommentLength);
        stream.ReadUInt16(mFileDiskNumber);
        stream.ReadUInt16(mInternalFileAttributes);
        stream.ReadUInt32(mExternalFileAttributes);
        stream.ReadUInt32(mRelativeLocalFileHeaderOffset);

        if (mLocalFileHeader.mFileNameLength)
        {
            mLocalFileHeader.mFileName.resize(mLocalFileHeader.mFileNameLength);
            stream.ReadBytes(reinterpret_cast<Uint8*>(&mLocalFileHeader.mFileName[0]), mLocalFileHeader.mFileName.size());
        }

        // TODO: Skip mLocalFileHeader.mExtraFieldLength
        // TODO: Skip mFileCommentLength
    }
};

ZipFileSystem::ZipFileSystem(const std::string& zipFile, IFileSystem& fs)
    : mFileName(zipFile)
{
    if (fs.FileExists(mFileName))
    {
        mStream = fs.Open(mFileName);
    }
}

bool ZipFileSystem::Init()
{
    if (!mStream)
    {
        LOG_ERROR("Failed to open " << mFileName);
        return false;
    }

    if (!LocateEndOfCentralDirectoryRecord())
    {
        LOG_ERROR("End of central directory record not found, this isn't a valid ZIP file");
        return false;
    }

    std::vector<CentralDirectoryRecord> records;
    records.resize(mEndOfCentralDirectoryRecord.mNumEntriesInCentaralDirectory);
    // TODO: Last entry is something else? ECDR?
    for (auto i = 0; i < mEndOfCentralDirectoryRecord.mNumEntriesInCentaralDirectory-1; i++)
    {
        records[i].DeSerialize(*mStream);

        Uint32 cdrMagic = 0;
        mStream->ReadUInt32(cdrMagic);
        if (cdrMagic != kCentralDirectory)
        {
            LOG_ERROR("Missing central directory record for item " << i);
            return false;
        }
    }

    for (const CentralDirectoryRecord& r : records)
    {
        LOG_INFO("File name: " << r.mLocalFileHeader.mFileName);
        
        // TODO: Get file data
        //r.mRelativeLocalFileHeaderOffset;
    }

    return true;
}

bool ZipFileSystem::LocateEndOfCentralDirectoryRecord()
{
    const size_t fileSize = mStream->Size();
    bool found = false;

    // The ECRD is at the end of the file, so start at the end of the file and work towards
    // the front looking for it. We start at end-22 since the size of an ECRD is 22 bytes.
    size_t searchPos = 22;

    // The max search size is the size of the structure and the max comment length, if there
    // isn't an ECDR within this range then its not a ZIP file.
    size_t kMaxSearchPos = 22 + std::numeric_limits<Uint16>::max();
    if (kMaxSearchPos > fileSize)
    {
        // But don't underflow on seeking
        kMaxSearchPos = fileSize;
    }

    do
    {
        // Keep moving backwards and see if we have the magic marker for an ECRD yet
        Uint32 magic = 0;
        mStream->Seek(fileSize - searchPos);
        mStream->ReadUInt32(magic);
        if (magic == kEndOfCentralDirectory)
        {
            // We do so check that the pos after the stucture + comment len == file size
            // and then seek to the central directory pos and check that it == correct magic
            mEndOfCentralDirectoryRecord.DeSerialize(*mStream);
            if (mEndOfCentralDirectoryRecord.mCommentSize + mStream->Pos() == fileSize)
            {
                mStream->Seek(mEndOfCentralDirectoryRecord.mCentralDirectoryStartOffset);

                Uint32 cdrMagic = 0;
                mStream->ReadUInt32(cdrMagic);
                if (cdrMagic == kCentralDirectory)
                {
                    // Must be a valid ZIP and we are now at the CDR location
                    return true;
                }
            }
        }
        else
        {
            // Didn't find the ECDR so keep going towards the start of the file
            searchPos--;
        }
    } while (!found || searchPos >= kMaxSearchPos);
    return false;
}

std::unique_ptr<Oddlib::IStream> ZipFileSystem::Open(const std::string& /*fileName*/)
{
    return nullptr;
}

std::vector<std::string> ZipFileSystem::EnumerateFiles(const std::string& /*directory*/, const char* /*filter*/)
{
    return std::vector<std::string>{};
}

bool ZipFileSystem::FileExists(const std::string& /*fileName*/)
{
    return false;
}

std::string ZipFileSystem::FsPath() const
{
    return mFileName;
}
