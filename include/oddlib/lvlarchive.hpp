#pragma once

#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include <fstream>
#include "SDL_types.h"

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

    class Exception : public std::exception
    {
    public:
        explicit Exception(const char* msg)
            : mMsg(msg)
        {

        }

        const char* what() const throw () override
        {
            return mMsg;
        }

    private:
        const char* mMsg;
    };

    class Stream
    {
    public:
        Stream(const std::string& fileName);
        void ReadUInt32(Uint32& output);
        void ReadBytes(Sint8* pDest, size_t destSize);
        void Seek(size_t pos);
    private:
        std::ifstream mStream;
    };

    class LvlArchive
    {
    public:
        class File;
        class FileChunk
        {
        public:
            FileChunk& operator = (const FileChunk&) const = delete;
            FileChunk(const FileChunk&) = delete;
            FileChunk(File& parent, Uint32 id) 
                : mParent(parent), mId(id) 
            {

            }
            Uint32 Id() const;
            std::vector<Uint8> ReadData() const;
        private:
            Uint32 mId = 0;
            File& mParent;
        };

        struct FileRecord;
        class File
        {
        public:
            File(const File&) = delete;
            File& operator = (const File&) = delete;
            File(LvlArchive& parent, const FileRecord& rec);
            const std::string& FileName() const;
            FileChunk* ChunkById(Uint32 id);
        private:
            void LoadChunks();
            std::string mFileName;
            std::vector<std::unique_ptr<FileChunk>> mChunks;
            LvlArchive& mParent;
        };

        explicit LvlArchive(const std::string& fileName);
        File* FileByName(const std::string& fileName);

        struct FileRecord
        {
            Uint32 iStartSector = 0;
            Uint32 iNumSectors = 0;
            Uint32 iFileSize = 0;
            // This array will not be NULL terminated
            Sint8 iFileNameBytes[KMaxLvlArchiveFileNameLength] = {};
        };

    private:
        struct Header
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

        void ReadHeader(Header& header);

        Stream mStream;
        std::vector<std::unique_ptr<File>> mFiles;
    };
}
