#include <sstream>
#include <algorithm>
#include <fstream>
#include <iterator>
#include "logger.hpp"
#include "oddlib/stream.hpp"
#include "oddlib/exceptions.hpp"

namespace Oddlib
{
    MemoryStream::MemoryStream(std::vector<u8>&& data)
    {
        mSize = data.size();
        auto s = std::make_unique<std::stringstream>();
        std::copy(data.begin(), data.end(), std::ostream_iterator<unsigned char>(*s));
        mStream = std::move(s);
        Seek(0);
        mName = "Memory buffer (" + std::to_string(mSize) + ") bytes";
        LOG_INFO(mName);
    }

    IStream* MemoryStream::Clone()
    {
        auto oldPos = Pos();

        Seek(0);
        std::vector<u8> streamData(Size());
        Read(streamData);
        Seek(oldPos);

        return new MemoryStream(std::move(streamData));
    }

    FileStream::FileStream(const std::string& fileName, ReadMode mode)
        : mMode(mode)
    {
        mName = fileName;

        auto s = std::make_unique<std::fstream>();

        std::ios_base::openmode flags = std::ios::binary | std::ios::ate;
        if (mMode == IStream::ReadMode::ReadOnly)
        {
            flags |= std::ios::in;
        }
        else if (mMode == IStream::ReadMode::ReadWrite)
        {
            flags |= std::ios::in | std::ios::out | std::ios::trunc;
        }

        s->open(fileName.c_str(), flags);
        if (!*s)
        {
            if (mMode == IStream::ReadMode::ReadOnly)
            {
                LOG_ERROR("File not found or couldn't be opened for reading " << fileName);
            }
            else
            {
                LOG_ERROR("File not found or couldn't be created for writing " << fileName);
            }
            throw Exception("File I/O error");
        }

        mSize = static_cast<size_t>(s->tellg());
        s->seekg(std::ios::beg);

        mStream = std::move(s);

        LOG_INFO("Opened " << fileName << " in mode " << static_cast<u32>(mode));
    }

    IStream* FileStream::Clone()
    {
        return new FileStream(mName, mMode);
    }

    template<class T>
    IStream* Stream<T>::Clone(u32 /*start*/, u32 /*size*/)
    {
        throw Exception("Sub clone not supported on direct file streams");
    }

    template<typename U, typename T>
    void DoRead(std::unique_ptr<U>& stream, T& output)
    {
        if (!stream->read(reinterpret_cast<char*>(&output), sizeof(output)))
        {
            throw Exception("Read failure");
        }
    }

    template<class T>
    void Stream<T>::ReadBytes(u8* pDest, size_t destSize)
    {
        if (!mStream->read(reinterpret_cast<char*>(pDest), destSize))
        {
            throw Exception("ReadBytes failure");
        }
    }

    template<class T>
    void Stream<T>::WriteBytes(const u8* pSrc, size_t srcSize)
    {
        if (!mStream->write(reinterpret_cast<const char*>(pSrc), srcSize))
        {
            throw Exception("WriteBytes failure");
        }
    }

    template<class T>
    void Stream<T>::Seek(size_t pos)
    {
        if (!mStream->seekg(pos))
        {
            throw Exception("Seek get failure");
        }

        if (!mStream->seekp(pos))
        {
            throw Exception("Seek put failure");
        }
    }

    template<class T>
    bool Stream<T>::AtEnd() const
    {
        const int c = mStream->peek();
        return (c == EOF);
    }

    template<class T>
    std::string Stream<T>::LoadAllToString()
    {
        Seek(0);
        std::string content
        { 
            std::istreambuf_iterator<char>(*mStream),
            std::istreambuf_iterator<char>()
        };
        return content;
    }

    template<class T>
    size_t Stream<T>::Pos() const
    {
        const size_t pos = static_cast<size_t>(mStream->tellg());
        return pos;
    }

    template<class T>
    size_t Stream<T>::Size() const
    {
        return mSize;
    }
}
