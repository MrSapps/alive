#include <algorithm>
#include "oddlib/lvlarchive.hpp"
#include "oddlib/exceptions.hpp"
#include "logger.hpp"
#include <fstream>

namespace Oddlib
{
    u32 LvlArchive::FileChunk::Id() const
    {
        return mId;
    }

    u32 LvlArchive::FileChunk::Type() const
    {
        return mType;
    }

    std::vector<u8> LvlArchive::FileChunk::ReadData() const
    {
        std::vector<u8> r(mDataSize);
        if (mDataSize > 0)
        {
            mStream.Seek(mFilePos);
            mStream.Read(r);
        }
        return r;
    }

    std::unique_ptr<Oddlib::IStream> LvlArchive::FileChunk::Stream() const
    {
        return std::make_unique<Oddlib::Stream>(ReadData());
    }
    
    bool LvlArchive::FileChunk::operator != (const FileChunk& rhs) const
    {
        return !(*this == rhs);
    }

    bool LvlArchive::FileChunk::operator == (const FileChunk& rhs) const
    {
        if (mId != rhs.Id())
        {
            return false;
        }

        if (mType != rhs.mType)
        {
            return false;
        }

        if (mDataSize != rhs.mDataSize)
        {
            return false;
        }

        return ReadData() == rhs.ReadData();
    }

    const std::string& LvlArchive::File::FileName() const
    {
        return mFileName;
    }

    // ===================================================================

    LvlArchive::File::File(IStream& stream, const LvlArchive::FileRecord& rec)
    {
        mFileName = std::string(
            reinterpret_cast<const char*>(rec.iFileNameBytes), 
            strnlen(reinterpret_cast<const char*>(rec.iFileNameBytes), sizeof(rec.iFileNameBytes)));

        stream.Seek(rec.iStartSector * kSectorSize);

        // Sound files are the only ones which are not chunk based
        if (string_util::ends_with(mFileName, ".VH") || string_util::ends_with(mFileName, ".VB"))
        {
            // Handle loading as a "blob" by inserting a dummy chunk
            mChunks.emplace_back(std::make_unique<FileChunk>(stream, 0, 0, rec.iFileSize));
            return;
        }

        LoadChunks(stream, rec.iFileSize);
    }

    LvlArchive::FileChunk* LvlArchive::File::ChunkById(u32 id)
    {
        LOG_INFO("Find chunk with id " << id);
        auto it = std::find_if(std::begin(mChunks), std::end(mChunks), [&] (std::unique_ptr<FileChunk>& chunk)
        {
            return chunk->Id() == id;
        });
        return it == std::end(mChunks) ? nullptr : it->get();
    }

    LvlArchive::FileChunk* LvlArchive::File::ChunkByType(u32 type)
    {
        LOG_INFO("Find chunk with type " << type);
        auto it = std::find_if(std::begin(mChunks), std::end(mChunks), [&](std::unique_ptr<FileChunk>& chunk)
        {
            return chunk->Type() == type;
        });
        return it == std::end(mChunks) ? nullptr : it->get();
    }

    void LvlArchive::File::SaveChunks()
    {
        for (const auto& chunk : mChunks)
        {
            std::ofstream o("chunk_" + std::to_string(chunk->Id()) + ".dat", std::ios::binary);
            if (o.is_open())
            {
                const auto data = chunk->ReadData();
                o.write(reinterpret_cast<const char*>(data.data()), data.size());
            }
        }
    }

    void LvlArchive::File::LoadChunks(IStream& stream, u32 fileSize)
    {
        while (stream.Pos() < (stream.Pos() + fileSize))
        {
            ChunkHeader header;
            stream.Read(header.iSize);
            stream.Read(header.iRefCount);
            stream.Read(header.iType);
            stream.Read(header.iId);

            const bool isEnd = header.iType == MakeType("End!");
            const u32 kChunkHeaderSize = sizeof(u32) * 4;

            if (!isEnd)
            {
                mChunks.emplace_back(std::make_unique<FileChunk>(stream, header.iType, header.iId, header.iSize - kChunkHeaderSize));
            }

            // Only move to next if the block isn't empty
            if (header.iSize)
            {
                stream.Seek((stream.Pos() + header.iSize) - kChunkHeaderSize);
            }

            // End! block, stop reading any more blocks
            if (isEnd)
            {
                break;
            }
        }
    }

    // ===================================================================

    LvlArchive::LvlArchive(const std::string& fileName)
        : mStream(std::make_unique<Stream>(fileName))
    {
        TRACE_ENTRYEXIT;
        Load();
    }

    LvlArchive::LvlArchive(std::unique_ptr<IStream> stream)
        : mStream(std::move(stream))
    {
        TRACE_ENTRYEXIT;
        Load();
    }

    LvlArchive::LvlArchive(std::vector<u8>&& data)
        : mStream(std::make_unique<Stream>(std::move(data)))
    {
        TRACE_ENTRYEXIT;
        Load();
    }

    LvlArchive::~LvlArchive()
    {
        TRACE_ENTRYEXIT;
        if (mStream)
        {
            LOG_INFO("Closing LVL: " << mStream->Name());
        }
    }

    void LvlArchive::Load()
    {
        // Read and validate the header
        LvlHeader header;
        ReadHeader(header);
        if (header.iNull1 != 0 || header.iNull2 != 0 || header.iMagic != MakeType("Indx"))
        {
            LOG_ERROR("Invalid LVL header");
            throw InvalidLvl("Invalid header");
        }

        // Read the file records
        std::vector<FileRecord> recs;
        recs.reserve(header.iNumFiles);
        for (auto i = 0u; i < header.iNumFiles; i++)
        {
            FileRecord rec;
            mStream->Read(rec.iFileNameBytes);
            mStream->Read(rec.iStartSector);
            mStream->Read(rec.iNumSectors);
            mStream->Read(rec.iFileSize);
            recs.emplace_back(rec);
        }

        for (const auto& rec : recs)
        {
            mFiles.emplace_back(std::make_unique<File>(*mStream, rec));
        }

        LOG_INFO("Loaded LVL '" << mStream->Name() << "' with " << header.iNumFiles << " files");
    }

    LvlArchive::File* LvlArchive::FileByName(const std::string& fileName)
    {
        LOG_INFO("Find file '" << fileName << "'");
        auto it = std::find_if(std::begin(mFiles), std::end(mFiles), [&](std::unique_ptr<File>& file)
        {
            return file->FileName() == fileName;
        });
        return it == std::end(mFiles) ? nullptr : it->get();
    }

    void LvlArchive::ReadHeader(LvlHeader& header)
    {
        mStream->Read(header.iFirstFileOffset);
        mStream->Read(header.iNull1);
        mStream->Read(header.iMagic);
        mStream->Read(header.iNull2);
        mStream->Read(header.iNumFiles);
        mStream->Read(header.iUnknown1);
        mStream->Read(header.iUnknown2);
        mStream->Read(header.iUnknown3);
    }
}
