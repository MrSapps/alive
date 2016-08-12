#pragma once

#include <string>
#include <vector>
#include <memory>
#include "SDL.h"
#include "string_util.hpp"
#include "oddlib/stream.hpp"
#include "oddlib/exceptions.hpp"

namespace Oddlib
{
    inline u32 MakeType(const char(&value)[5])
    {
        return(value[0]) | (value[1] << 8) | (value[2] << 16) | (value[3] << 24);
    }

    const static u32 KMaxLvlArchiveFileNameLength = 12;
    const static u32 kSectorSize = 2048;

    class InvalidLvl : public Exception
    {
    public:
        explicit InvalidLvl(const char* msg) : Exception(msg) { }
    };

    class LvlArchive
    {
    public:
        class FileChunk
        {
        public:
            FileChunk& operator = (const FileChunk&) const = delete;
            FileChunk(const FileChunk&) = delete;
            FileChunk(IStream& stream, u32 type, u32 id, u32 dataSize)
                : mStream(stream), mId(id), mType(type), mDataSize(dataSize)
            {
                mFilePos = static_cast<u32>(stream.Pos());
            }
            u32 Id() const;
            u32 Type() const;
            std::vector<u8> ReadData() const;
            std::unique_ptr<Oddlib::IStream> Stream() const;
            bool operator != (const FileChunk& rhs) const;
            bool operator == (const FileChunk& rhs) const;
        private:
            IStream& mStream;
            u32 mId = 0;
            u32 mType = 0;
            u32 mFilePos = 0;
            u32 mDataSize = 0; 
        };

        struct FileRecord;
        class File
        {
        public:
            File(const File&) = delete;
            File& operator = (const File&) = delete;
            File(IStream& stream, const FileRecord& rec);
            const std::string& FileName() const;
            FileChunk* ChunkById(u32 id);
            FileChunk* ChunkByIndex(u32 index) { return mChunks[index].get(); }
            FileChunk* ChunkByType(u32 type);
            size_t ChunkCount() const { return mChunks.size(); }
            // Debugging feature
            void SaveChunks();
        private:
            void LoadChunks(IStream& stream, u32 fileSize);
            std::string mFileName;
            std::vector<std::unique_ptr<FileChunk>> mChunks;
        };

        explicit LvlArchive(const std::string& fileName);
        explicit LvlArchive(std::vector<u8>&& data);
        explicit LvlArchive(std::unique_ptr<IStream> stream);
        ~LvlArchive();

        File* FileByName(const std::string& fileName);
        File* FileByIndex(u32 index) { return mFiles[index].get(); }
        u32 FileCount() const { return static_cast<u32>(mFiles.size()); }
        struct FileRecord
        {
            u32 iStartSector = 0;
            u32 iNumSectors = 0;
            u32 iFileSize = 0;
            // This array will not be NULL terminated
            s8 iFileNameBytes[KMaxLvlArchiveFileNameLength] = {};
        };

    private:
        void Load();

        struct LvlHeader
        {
            u32 iFirstFileOffset = 0;
            u32 iNull1 = 0;
            u32 iMagic = 0; // (Indx)
            u32 iNull2 = 0;
            u32 iNumFiles = 0;
            u32 iUnknown1 = 0;
            u32 iUnknown2 = 0;
            u32 iUnknown3 = 0;
        };

        struct ChunkHeader
        {
            u32 iSize = 0;          // Size inc header
            u32 iRefCount = 0;      // Ref count, should be zero when loaded
            u32 iType = 0;
            u32 iId = 0;            // Id
        };

        void ReadHeader(LvlHeader& header);

        std::unique_ptr<IStream> mStream;
        std::vector<std::unique_ptr<File>> mFiles;
    };
}
