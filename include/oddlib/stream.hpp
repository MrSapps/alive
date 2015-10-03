#pragma once

#include <vector>
#include <iostream>
#include <memory>
#include "SDL.h"

namespace Oddlib
{
    class IStream
    {
    public:
        virtual ~IStream() = default;
        virtual IStream* Clone() = 0;
        virtual IStream* Clone(Uint32 start, Uint32 size) = 0;
        virtual void ReadUInt8(Uint8& output) = 0;
        virtual void ReadUInt32(Uint32& output) = 0;
        virtual void ReadUInt16(Uint16& output) = 0;
        virtual void ReadSInt16(Sint16& output) = 0;
        virtual void ReadBytes(Sint8* pDest, size_t destSize) = 0;
        virtual void ReadBytes(Uint8* pDest, size_t destSize) = 0;
        virtual void Seek(size_t pos) = 0;
        virtual size_t Pos() const = 0;
        virtual size_t Size() const = 0;
        virtual bool AtEnd() const = 0;
        virtual const std::string& Name() const = 0;
        virtual std::string LoadAllToString() = 0;
    };

    class Stream : public IStream
    {
    public:
        explicit Stream(const std::string& fileName);
        explicit Stream(std::vector<Uint8>&& data);
        virtual IStream* Clone() override;
        virtual IStream* Clone(Uint32 start, Uint32 size) override;
        virtual void ReadUInt8(Uint8& output) override;
        virtual void ReadUInt32(Uint32& output) override;
        virtual void ReadUInt16(Uint16& output) override;
        virtual void ReadSInt16(Sint16& output) override;
        virtual void ReadBytes(Sint8* pDest, size_t destSize) override;
        virtual void ReadBytes(Uint8* pDest, size_t destSize) override;
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
