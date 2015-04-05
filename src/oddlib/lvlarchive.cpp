#include "oddlib/lvlarchive.hpp"
#include "logger.hpp"

namespace Oddlib
{
    Stream::Stream(const std::string& fileName)
    {
        mStream.open(fileName, std::ios::binary);
        if (!mStream.is_open())
        {
            LOG_ERROR("Lvl file not found %s", fileName.c_str());
            throw Exception("File not found");
        }
    }

    void Stream::ReadUInt32(Uint32& output)
    {
        if (!mStream.read(reinterpret_cast<char*>(&output), sizeof(output)))
        {
            throw Exception("ReadUInt32 failure");
        }
    }

    void Stream::ReadBytes(Sint8* pDest, size_t destSize)
    {
        if (!mStream.read(reinterpret_cast<char*>(pDest), destSize))
        {
            throw Exception("ReadBytes failure");
        }
    }

    void Stream::ReadBytes(Uint8* pDest, size_t destSize)
    {
        if (!mStream.read(reinterpret_cast<char*>(pDest), destSize))
        {
            throw Exception("ReadBytes failure");
        }
    }

    void Stream::Seek(size_t pos)
    {
        if (!mStream.seekg(pos))
        {
            throw Exception("Seek failure");
        }
    }

    size_t Stream::Pos() const
    {
        return static_cast<size_t>(mStream.tellg());
    }

    // ===================================================================

    Uint32 LvlArchive::FileChunk::Id() const
    {
        return mId;
    }

    Uint32 LvlArchive::FileChunk::Type() const
    {
        return mType;
    }

    std::vector<Uint8> LvlArchive::FileChunk::ReadData() const
    {
        std::vector<Uint8> r(mDataSize);
        if (mDataSize > 0)
        {
            mStream.Seek(mFilePos);
            mStream.ReadBytes(r.data(), r.size());
        }
        return r;
    }

    const std::string& LvlArchive::File::FileName() const
    {
        return mFileName;
    }

    // ===================================================================

    LvlArchive::File::File(Stream& stream, const LvlArchive::FileRecord& rec)
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

    LvlArchive::FileChunk* LvlArchive::File::ChunkById(Uint32 id)
    {
        LOG_INFO("Find chunk with id %d", id);
        auto it = std::find_if(std::begin(mChunks), std::end(mChunks), [&] (std::unique_ptr<FileChunk>& chunk)
        {
            return chunk->Id() == id;
        });
        return it == std::end(mChunks) ? nullptr : it->get();
    }

    void LvlArchive::File::LoadChunks(Stream& stream, Uint32 fileSize)
    {
        while (stream.Pos() < stream.Pos() + fileSize)
        {
            ChunkHeader header;
            stream.ReadUInt32(header.iSize);
            stream.ReadUInt32(header.iRefCount);
            stream.ReadUInt32(header.iType);
            stream.ReadUInt32(header.iId);

            const bool isEnd = header.iType == MakeType('E', 'n', 'd', '!');
            const Uint32 kChunkHeaderSize = sizeof(Uint32) * 4;

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
        : mStream(fileName)
    {
        TRACE_ENTRYEXIT;

        // Read and validate the header
        LvlHeader header;
        ReadHeader(header);
        if (header.iNull1 != 0 && header.iNull2 != 0 && header.iMagic != MakeType('I', 'n', 'd', 'x'))
        {
            LOG_ERROR("Invalid LVL header");
            throw Exception("Invalid header");
        }

        // Read the file records
        std::vector<FileRecord> recs;
        recs.reserve(header.iNumFiles);
        for (auto i = 0u; i < header.iNumFiles; i++)
        {
            FileRecord rec;
            mStream.ReadBytes(&rec.iFileNameBytes[0], sizeof(rec.iFileNameBytes));
            mStream.ReadUInt32(rec.iStartSector);
            mStream.ReadUInt32(rec.iNumSectors);
            mStream.ReadUInt32(rec.iFileSize);
            recs.emplace_back(rec);
        }

        for (const auto& rec : recs)
        {
            mFiles.emplace_back(std::make_unique<File>(mStream, rec));
        }

        LOG_INFO("Loaded LVL '%s' with %d files", fileName.c_str(), header.iNumFiles);
    }

    LvlArchive::File* LvlArchive::FileByName(const std::string& fileName)
    {
        LOG_INFO("Find file '%s'", fileName.c_str());
        auto it = std::find_if(std::begin(mFiles), std::end(mFiles), [&](std::unique_ptr<File>& file)
        {
            return file->FileName() == fileName;
        });
        return it == std::end(mFiles) ? nullptr : it->get();
    }

    void LvlArchive::ReadHeader(LvlHeader& header)
    {
        mStream.ReadUInt32(header.iFirstFileOffset);
        mStream.ReadUInt32(header.iNull1);
        mStream.ReadUInt32(header.iMagic);
        mStream.ReadUInt32(header.iNull2);
        mStream.ReadUInt32(header.iNumFiles);
        mStream.ReadUInt32(header.iUnknown1);
        mStream.ReadUInt32(header.iUnknown2);
        mStream.ReadUInt32(header.iUnknown3);
    }
}
