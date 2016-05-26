#pragma once

#include <SDL_stdinc.h>
#include <memory>
#include "zipfilesystem.hpp"
#include "unzip.h"
#include "ioapi.h"
#include "iowin32.h"

#undef max

const Uint32 kEndOfCentralDirectory = 0x06054b50;
struct EndOfCentralDirectoryRecord
{
    Uint16 mThisDiskNumber;
    Uint16 mStartCentralDirectoryDiskNumber;
    Uint16 mNumEntriesInCentaralDirectoryOnThisDisk;
    Uint16 mNumEntriesInCentaralDirectory;
    Uint32 mCentralDirectorySize;
    Uint32 mCentralDirectoryStartOffset;
    Uint16 mCommentSize;
    // comment
};

// DataDescriptor signature = 0x08074b50
struct DataDescriptor
{
    Uint32 mCrc32;
    Uint32 mCompressedSize;
    Uint32 mUnCompressedSize;
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
    // file name
    // extra field
};

// Central directory file header signature = 0x02014b50
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

    LocateEndOfCentralDirectoryRecord();

    return true;
}

void ZipFileSystem::LocateEndOfCentralDirectoryRecord()
{
    const size_t fileSize = mStream->Size();
    bool found = false;
    size_t searchPos = 22;
    const size_t kMaxSearchPos = 22 + std::numeric_limits<Uint16>::max();
    do
    {
        mStream->Seek(fileSize - searchPos);

        Uint32 magic = 0;
        mStream->ReadUInt32(magic);
        if (magic == kEndOfCentralDirectory)
        {
            // TODO: Actually need to check that pos + comment len == file size
            // and seek central directory pos == correct magic
            found = true;
        }
        else
        {
            searchPos--;
        }
    } while (!found || searchPos >= kMaxSearchPos);
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
