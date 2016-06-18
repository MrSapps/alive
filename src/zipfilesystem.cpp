#include <SDL_stdinc.h>
#include <memory>
#include "zipfilesystem.hpp"
#include "libdeflate.h"

#undef max

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
        Uint32 cdrMagic = 0;
        mStream->ReadUInt32(cdrMagic);
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
    bool found = false;

    // The ECRD is at the end of the file, so start at the end of the file and work towards
    // the front looking for it. We start at end-22 since the size of an ECRD is 22 bytes.
    size_t searchPos = kEndOfCentralDirectoryRecordSizeWithMagic;

    // The max search size is the size of the structure and the max comment length, if there
    // isn't an ECDR within this range then its not a ZIP file.
    size_t kMaxSearchPos = kEndOfCentralDirectoryRecordSizeWithMagic + std::numeric_limits<Uint16>::max();
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

            mStream->Seek(mEndOfCentralDirectoryRecord.mCentralDirectoryStartOffset);

            Uint32 cdrMagic = 0;
            mStream->ReadUInt32(cdrMagic);
            if (cdrMagic == kCentralDirectory)
            {
                // Must be a valid ZIP and we are now at the CDR location
                mStream->Seek(mStream->Pos() - sizeof(cdrMagic));
                return true;
            }

            else
            {
                searchPos++;
            }
        }
        else
        {
            // Didn't find the ECDR so keep going towards the start of the file
            searchPos++;
        }
    } while (!found || searchPos >= kMaxSearchPos);
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
    Uint32 magic = 0;
    mStream->ReadUInt32(magic);
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
        std::vector<Uint8> buffer(compressedSize);
        std::vector<Uint8> out(r.mLocalFileHeader.mDataDescriptor.mUnCompressedSize);
        size_t actualOut = 0;

        mStream->ReadBytes(buffer.data(), buffer.size());

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

        return std::make_unique<Oddlib::Stream>(std::move(out));
    }
    return std::make_unique<Oddlib::Stream>(std::vector<Uint8>());
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
