#include <SDL_stdinc.h>
#include <memory>
#include <limits>
#include "zipfilesystem.hpp"
#include "libdeflate.h"
#include "oddlib/stream.hpp"
#include "logger.hpp"
#include "oddlib/exceptions.hpp"

#undef max

void ZipFileSystem::EndOfCentralDirectoryRecord::DeSerialize(Oddlib::IStream& stream)
{
    stream.Read(mThisDiskNumber);
    stream.Read(mStartCentralDirectoryDiskNumber);
    stream.Read(mNumEntriesInCentaralDirectoryOnThisDisk);
    stream.Read(mNumEntriesInCentaralDirectory);
    stream.Read(mCentralDirectorySize);
    stream.Read(mCentralDirectoryStartOffset);
    stream.Read(mCommentSize);
}

void ZipFileSystem::DataDescriptor::DeSerialize(Oddlib::IStream& stream)
{
    stream.Read(mCrc32);
    stream.Read(mCompressedSize);
    stream.Read(mUnCompressedSize);
}

void ZipFileSystem::LocalFileHeader::DeSerialize(Oddlib::IStream& stream)
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

void ZipFileSystem::CentralDirectoryRecord::DeSerialize(Oddlib::IStream& stream)
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

    if (!LoadCentralDirectoryRecords())
    {
        LOG_ERROR("Failed to load file/directory records");
        return false;
    }

    return true;
}

bool ZipFileSystem::LoadCentralDirectoryRecords()
{
    mRecords.resize(mEndOfCentralDirectoryRecord.mNumEntriesInCentaralDirectory);
    for (auto i = 0; i < mEndOfCentralDirectoryRecord.mNumEntriesInCentaralDirectory; i++)
    {
        u32 cdrMagic = 0;
        mStream->Read(cdrMagic);
        if (cdrMagic != kCentralDirectory)
        {
            LOG_ERROR("Missing central directory record for item " << i);
            return false;
        }
        mRecords[i].DeSerialize(*mStream);
    }

    // TODO: Sort mRecords by name for faster file look up and directory enumeration

    return true;
}

bool ZipFileSystem::LocateEndOfCentralDirectoryRecord()
{
    const size_t fileSize = mStream->Size();

    // The max search size is the size of the structure and the max comment length, if there
    // isn't an ECDR within this range then its not a ZIP file.
    size_t kMaxSearchPos = kEndOfCentralDirectoryRecordSizeWithMagic + std::numeric_limits<u16>::max();
    if (kMaxSearchPos > fileSize)
    {
        // But don't underflow on seeking
        kMaxSearchPos = fileSize;
    }

    // Move to the earliest possible start pos for the ECDR
    const size_t baseOffset = fileSize - kMaxSearchPos;
    mStream->Seek(baseOffset);
    std::string buffer;
    buffer.resize(kMaxSearchPos);
    mStream->Read(buffer);

    const std::string needle = { 'P', 'K', 0x05, 0x06 }; // AKA kEndOfCentralDirectory

    size_t hint = 0;
    do
    {
        hint = buffer.find(needle, hint);
        if (hint != std::string::npos)
        {
            mStream->Seek(baseOffset + hint);

            u32 magic = 0;
            mStream->Read(magic);
            if (magic == kEndOfCentralDirectory)
            {
                // We do so check that the pos after the stucture + comment len == file size
                // and then seek to the central directory pos and check that it == correct magic
                mEndOfCentralDirectoryRecord.DeSerialize(*mStream);
                if (mEndOfCentralDirectoryRecord.mCentralDirectoryStartOffset < fileSize - sizeof(u32))
                {
                    mStream->Seek(mEndOfCentralDirectoryRecord.mCentralDirectoryStartOffset);
                    u32 cdrMagic = 0;
                    mStream->Read(cdrMagic);
                    if (cdrMagic == kCentralDirectory)
                    {
                        // Must be a valid ZIP and we are now at the CDR location
                        mStream->Seek(mStream->Pos() - sizeof(cdrMagic));
                        return true;
                    }
                }
            }
            hint++;
        }
    } while (hint != std::string::npos);

    return false;
}

std::unique_ptr<Oddlib::IStream> ZipFileSystem::Open(const std::string& fileName)
{
    size_t idx = 0;
    bool found = false;
    for (size_t i = 0; i < mRecords.size(); i++)
    {
        CentralDirectoryRecord& r = mRecords[i];
        if (r.mLocalFileHeader.mFileName == fileName)
        {
            idx = i;
            found = true;
            break;
        }
    }

    if (!found)
    {
        return nullptr;
    }

    CentralDirectoryRecord& r = mRecords[idx];

    mStream->Seek(r.mRelativeLocalFileHeaderOffset);
    u32 magic = 0;
    mStream->Read(magic);
    if (magic != kLocalFileHeader)
    {
        LOG_ERROR("Local file header missing");
        return nullptr;
    }

    // The other copy of the local file header might have a comment with a different length etc, so have to check again
    LocalFileHeader localHeader;
    localHeader.DeSerialize(*mStream);
    mStream->Seek(mStream->Pos() + localHeader.mFileNameLength + localHeader.mExtraFieldLength);

    enum GeneralPurposeFlags
    {
        // Bits 1,2 depend on compression method
        eExternalDataDescriptor = 1 << 3,
        eEhancedDeflating = 1 << 4,
        eCompressedPatchData = 1 << 5,
        eStrongEncryption = 1 << 6,
        eUtf8 = 1 << 11,
        eEncryptedCentralDirectory = 1 << 13
    };

    if (r.mLocalFileHeader.mGeneralPurposeFlags & eExternalDataDescriptor)
    {
        LOG_ERROR("External data descriptors not supported");
        return nullptr;
    }
    else if (r.mLocalFileHeader.mGeneralPurposeFlags & eCompressedPatchData)
    {
        LOG_ERROR("Compressed patch data not supported");
        return nullptr;
    }
    else if (r.mLocalFileHeader.mGeneralPurposeFlags & eStrongEncryption)
    {
        LOG_ERROR("Strong encryption not supported");
        return nullptr;
    }
    else if (r.mLocalFileHeader.mGeneralPurposeFlags & eUtf8)
    {
        LOG_ERROR("UTF8 names not supported");
        return nullptr;
    }
    else if (r.mLocalFileHeader.mGeneralPurposeFlags & eEncryptedCentralDirectory)
    {
        LOG_ERROR("Encrypted central directory not supported");
        return nullptr;
    }

    enum CompressionMethods
    {
        eNone = 0,
        eShrunk = 1,
        eFactor1 = 3,
        eFactor2 = 3,
        eFactor3 = 4,
        eFactor4 = 5,
        eImploded = 6,
        eDeflate = 8,
        eDeflate64 = 9,
        eOldIBMTERSE = 10,
        eBZip2 = 12,
        eLZMA = 14,
        eNewIBMTERSE = 18,
        eWavePack = 97,
        ePPMdVersionIR1 = 98
    };

    if (r.mLocalFileHeader.mCompressionMethod != eDeflate &&
        r.mLocalFileHeader.mCompressionMethod != eNone)
    {
        LOG_ERROR("Unsupported compression method: " << r.mLocalFileHeader.mCompressionMethod);
        return nullptr;
    }

    // TODO: Wrap a stream around the compressed data, loading the whole thing is memory heavy and slow
    auto compressedSize = r.mLocalFileHeader.mDataDescriptor.mCompressedSize;
    if (compressedSize > 0)
    {
        std::vector<u8> buffer(compressedSize);
        std::vector<u8> out(r.mLocalFileHeader.mDataDescriptor.mUnCompressedSize);
        size_t actualOut = 0;

        mStream->Read(buffer);

        if (r.mLocalFileHeader.mCompressionMethod == eDeflate)
        {
            deflate_decompressor* decompressor = deflate_alloc_decompressor();
            decompress_result result = deflate_decompress(decompressor, buffer.data(), buffer.size(), out.data(), out.size(), &actualOut);
            switch (result)
            {
            case DECOMPRESS_BAD_DATA:
            case DECOMPRESS_INSUFFICIENT_SPACE:
            case DECOMPRESS_SHORT_OUTPUT:
                deflate_free_decompressor(decompressor);
                return nullptr;

            case DECOMPRESS_SUCCESS:
                break;
            }
            deflate_free_decompressor(decompressor);
        }
        else
        {
            out = buffer;
        }

        return std::make_unique<Oddlib::MemoryStream>(std::move(out));
    }
    return std::make_unique<Oddlib::MemoryStream>(std::vector<u8>());
}

std::unique_ptr<Oddlib::IStream> ZipFileSystem::Create(const std::string& /*fileName*/)
{
    TRACE_ENTRYEXIT;
    throw Oddlib::Exception("Create is not implemented");
}

std::vector<std::string> ZipFileSystem::EnumerateFiles(const std::string& directory, const char* filter)
{
    std::string dir = directory;
    NormalizePath(dir);

    std::vector<std::string> ret;
    std::string strFilter = filter;
    for (size_t i = 0; i < mRecords.size(); i++)
    {
        CentralDirectoryRecord& r = mRecords[i];
        DirectoryAndFileName dirAndFileName(r.mLocalFileHeader.mFileName);

        if (dirAndFileName.mDir == dir)
        {
            if (WildCardMatcher(dirAndFileName.mFile, strFilter, IFileSystem::IgnoreCase))
            {
                ret.emplace_back(dirAndFileName.mFile);
            }
        }
    }
    return ret;
}

std::vector<std::string> ZipFileSystem::EnumerateFolders(const std::string& /*directory*/)
{
    LOG_ERROR("Not implemented");
    abort();
}

bool ZipFileSystem::FileExists(const std::string& fileName)
{
    for (size_t i = 0; i < mRecords.size(); i++)
    {
        CentralDirectoryRecord& r = mRecords[i];
        if (r.mLocalFileHeader.mFileName == fileName)
        {
            return true;
        }
    }
    return false;
}

std::string ZipFileSystem::FsPath() const
{
    return mFileName;
}
