#pragma once

#include <vector>
#include <iostream>
#include <memory>
#include <array>
#include <fstream>
#include <sstream>
#include "SDL.h"
#include "proxy_sol.hpp"
#include "types.hpp"

namespace Oddlib
{
    class IStream
    {
    public:
        static inline void RegisterLuaBindings(sol::state& state);

        enum class ReadMode
        {
            ReadOnly,
            ReadWrite
        };

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

        // Read any fundamental type
        template<class T>
        void Read(T& type)
        {
            static_assert(std::is_fundamental<T>::value, "Can only read fundamental types");
            ReadBytes(reinterpret_cast<u8*>(&type), sizeof(type));
        }

        // Read a string
        void Read(std::string& type)
        {
            ReadBytes(reinterpret_cast<u8*>(&type[0]), type.size());
        }

        // Read any vector of fundamental type
        template<class T>
        void Read(std::vector<T>& type)
        {
            static_assert(std::is_fundamental<T>::value, "Can only read vectors of fundamental types");
            ReadBytes(reinterpret_cast<u8*>(type.data()), sizeof(T)*type.size());
        }

        // Read any std::array of fundamental type
        template<class T, std::size_t count>
        void Read(std::array<T, count>& type)
        {
            static_assert(std::is_fundamental<T>::value, "Can only read vectors of fundamental types");
            ReadBytes(reinterpret_cast<u8*>(type.data()), sizeof(T)*type.size());
        }

        // Read any fixed array of fundamental type
        template<typename T, std::size_t count>
        void Read(T(&value)[count])
        {
            static_assert(std::is_fundamental<T>::value, "Can only read fundamental types");
            ReadBytes(reinterpret_cast<u8*>(&value[0]), sizeof(T)* count);
        }

        // Write a string
        void Write(const std::string& type)
        {
            WriteBytes(reinterpret_cast<const u8*>(&type[0]), type.size());
        }

        virtual ~IStream() = default;
        virtual IStream* Clone() = 0;
        virtual IStream* Clone(u32 start, u32 size) = 0;
        virtual void ReadBytes(u8* pDest, size_t destSize) = 0;
        virtual void WriteBytes(const u8* pSrc, size_t srcSize) = 0;
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


    inline u16 ReadU16(IStream& stream)
    {
        u16 ret = 0;
        stream.Read(ret);
        return ret;
    }

    inline u32 ReadU32(IStream& stream)
    {
        u32 ret = 0;
        stream.Read(ret);
        return ret;
    }

    inline u8 ReadU8(IStream& stream)
    {
        u8 ret = 0;
        stream.Read(ret);
        return ret;
    }

    /*static*/ inline void IStream::RegisterLuaBindings(sol::state& state)
    {
        state.new_usertype<IStream>("IStream",
            "ReadU32", &ReadU32,
            "ReadU16", &ReadU16
            );
    }


    template<class T = std::ifstream>
    class Stream : public IStream
    {
    public:
        virtual IStream* Clone(u32 start, u32 size) override;
        virtual void ReadBytes(u8* pDest, size_t destSize) override;
        virtual void WriteBytes(const u8* pDest, size_t destSize) override;
        virtual void Seek(size_t pos) override;
        virtual size_t Pos() const override;
        virtual size_t Size() const override;
        virtual bool AtEnd() const override;
        virtual const std::string& Name() const override { return mName; }
        virtual std::string LoadAllToString() override;
    protected:
        Stream() = default;
        mutable std::unique_ptr<T> mStream;
        size_t mSize = 0;
        std::string mName;
    };

    class MemoryStream : public Stream<std::stringstream>
    {
    public:
        explicit MemoryStream(std::vector<u8>&& data);
        virtual IStream* Clone() override;
        virtual IStream* Clone(u32 start, u32 size) override { return Stream<std::stringstream>::Clone(start, size); }
    };

    class FileStream :public Stream<std::fstream>
    {
    public:
        explicit FileStream(const std::string& fileName, ReadMode mode);
        virtual IStream* Clone() override;
        virtual IStream* Clone(u32 start, u32 size) override { return Stream<std::fstream>::Clone(start, size); }
    private:
        ReadMode mMode = IStream::ReadMode::ReadOnly;
    };
}
