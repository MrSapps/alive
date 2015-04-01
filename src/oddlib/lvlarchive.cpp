#include "oddlib/lvlarchive.hpp"
#include "logger.h"

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
        mStream.read(reinterpret_cast<char*>(&output), sizeof(output));
    }

    void Stream::ReadBytes(Sint8* pDest, size_t destSize)
    {
        mStream.read(reinterpret_cast<char*>(pDest), destSize);
    }

    void Stream::Seek(size_t pos)
    {
        // TODO: Throw if fail
        mStream.seekg(pos);
    }

    // ===================================================================

    Uint32 LvlArchive::FileChunk::Id() const
    {
        return mId;
    }

    const std::string& LvlArchive::File::FileName() const
    {
        return mFileName;
    }

    // ===================================================================

    LvlArchive::File::File(LvlArchive& parent, const LvlArchive::FileRecord& rec)
        : mParent(parent)
    {
        //mFileName = std::string(&rec.iFileNameBytes[0], strnlen(rec.iFileNameBytes, sizeof(rec.iFileNameBytes)));
        //parent.mStream.Seek(rec.iStartSector * kSectorSize);
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

    // ===================================================================

    LvlArchive::LvlArchive(const std::string& fileName)
        : mStream(fileName)
    {
        TRACE_ENTRYEXIT;

        // Read and validate the header
        Header header;
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
            mFiles.emplace_back(std::make_unique<File>(*this, rec));
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

    void LvlArchive::ReadHeader(Header& header)
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
