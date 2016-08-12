#pragma once

#include <vector>
#include <iostream>
#include <memory>
#include <fstream>
#include "SDL.h"
#include "types.hpp"

namespace Oddlib
{
    class IStream
    {
    public:
        static std::vector<u8> ReadAll(IStream& stream)
        {
            const auto oldPos = stream.Pos();
            stream.Seek(0);
            const auto size = stream.Size();

            std::vector<u8> allStreamBytes(size);
            stream.ReadBytes(allStreamBytes.data(), allStreamBytes.size());
            stream.Seek(oldPos);
            return allStreamBytes;
        }

        virtual ~IStream() = default;
        virtual IStream* Clone() = 0;
        virtual IStream* Clone(u32 start, u32 size) = 0;
        virtual void ReadUInt8(u8& output) = 0;
        virtual void ReadUInt32(u32& output) = 0;
        virtual void ReadUInt16(u16& output) = 0;
        virtual void ReadSInt16(s16& output) = 0;
        virtual void ReadBytes(s8* pDest, size_t destSize) = 0;
        virtual void ReadBytes(u8* pDest, size_t destSize) = 0;
        virtual void Seek(size_t pos) = 0;
        virtual size_t Pos() const = 0;
        virtual size_t Size() const = 0;
        virtual bool AtEnd() const = 0;
        virtual const std::string& Name() const = 0;
        virtual std::string LoadAllToString() = 0;
        
        // Debug helper to write all of the stream to a file as a binary blob
        bool BinaryDump(const std::string& fileName)
        {
            std::ofstream s;
            s.open(fileName.c_str(), std::ios::binary);
            if (!s.is_open())
            {
                return false;
            }
            
            auto allStreamBytes = ReadAll(*this);
            s.write(reinterpret_cast<const char*>(allStreamBytes.data()), allStreamBytes.size());
            return true;
        }
    };

    inline u16 ReadUint16(IStream& stream)
    {
        u16 ret = 0;
        stream.ReadUInt16(ret);
        return ret;
    }

    inline u32 ReadUint32(IStream& stream)
    {
        u32 ret = 0;
        stream.ReadUInt32(ret);
        return ret;
    }

    inline u8 ReadUInt8(IStream& stream)
    {
        u8 ret = 0;
        stream.ReadUInt8(ret);
        return ret;
    }

    class Stream : public IStream
    {
    public:
        explicit Stream(const std::string& fileName);
        explicit Stream(std::vector<u8>&& data);
        virtual IStream* Clone() override;
        virtual IStream* Clone(u32 start, u32 size) override;
        virtual void ReadUInt8(u8& output) override;
        virtual void ReadUInt32(u32& output) override;
        virtual void ReadUInt16(u16& output) override;
        virtual void ReadSInt16(s16& output) override;
        virtual void ReadBytes(s8* pDest, size_t destSize) override;
        virtual void ReadBytes(u8* pDest, size_t destSize) override;
        virtual void Seek(size_t pos) override;
        virtual size_t Pos() const override;
        virtual size_t Size() const override;
        virtual bool AtEnd() const override;
        virtual const std::string& Name() const override { return mName; }
        virtual std::string LoadAllToString() override;
    private:
        mutable std::unique_ptr<std::istream> mStream;
        size_t mSize = 0;
        std::string mName;
        bool mFromMemoryBuffer = false;
    };
}
