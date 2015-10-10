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
    inline Uint32 MakeType(Uint8 b4, Uint8 b3, Uint8 b2, Uint8 b1)
    {
        return (static_cast<Uint32> (b1) << 24) +
               (static_cast<Uint32> (b2) << 16) + 
               (static_cast<Uint32> (b3) << 8) + 
                static_cast<Uint32> (b4);
    }

    const static Uint32 KMaxLvlArchiveFileNameLength = 12;
    const static Uint32 kSectorSize = 2048;

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
            enum eTypes
            {

            };

            FileChunk& operator = (const FileChunk&) const = delete;
            FileChunk(const FileChunk&) = delete;
            FileChunk(IStream& stream, Uint32 type, Uint32 id, Uint32 dataSize)
                : mStream(stream), mId(id), mType(type), mDataSize(dataSize)
            {
                mFilePos = static_cast<Uint32>(stream.Pos());
            }
            Uint32 Id() const;
            Uint32 Type() const;
            std::vector<Uint8> ReadData() const;
            std::unique_ptr<Oddlib::IStream> Stream() const;
        private:
            IStream& mStream;
            Uint32 mId = 0;
            Uint32 mType = 0;
            Uint32 mFilePos = 0;
            Uint32 mDataSize = 0; 
        };

        struct FileRecord;
        class File
        {
        public:
            File(const File&) = delete;
            File& operator = (const File&) = delete;
            File(IStream& stream, const FileRecord& rec);
            const std::string& FileName() const;
            FileChunk* ChunkById(Uint32 id);
            FileChunk* ChunkByIndex(Uint32 index) { return mChunks[index].get(); }
            size_t ChunkCount() const { return mChunks.size(); }
            // Deugging feature
            void SaveChunks();
        private:
            void LoadChunks(IStream& stream, Uint32 fileSize);
            std::string mFileName;
            std::vector<std::unique_ptr<FileChunk>> mChunks;
        };

        explicit LvlArchive(const std::string& fileName);
        explicit LvlArchive(std::vector<Uint8>&& data);
        explicit LvlArchive(std::unique_ptr<IStream> stream);

        File* FileByName(const std::string& fileName);
        Uint32 FileCount() const { return static_cast<Uint32>(mFiles.size()); }
        struct FileRecord
        {
            Uint32 iStartSector = 0;
            Uint32 iNumSectors = 0;
            Uint32 iFileSize = 0;
            // This array will not be NULL terminated
            Sint8 iFileNameBytes[KMaxLvlArchiveFileNameLength] = {};
        };

    private:
        void Load();

        struct LvlHeader
        {
            Uint32 iFirstFileOffset = 0;
            Uint32 iNull1 = 0;
            Uint32 iMagic = 0; // (Indx)
            Uint32 iNull2 = 0;
            Uint32 iNumFiles = 0;
            Uint32 iUnknown1 = 0;
            Uint32 iUnknown2 = 0;
            Uint32 iUnknown3 = 0;
        };

        struct ChunkHeader
        {
            Uint32 iSize = 0;          // Size inc header
            Uint32 iRefCount = 0;      // Ref count, should be zero when loaded
            Uint32 iType = 0;
            Uint32 iId = 0;            // Id
        };

        void ReadHeader(LvlHeader& header);

        std::unique_ptr<IStream> mStream;
        std::vector<std::unique_ptr<File>> mFiles;
    };
}
