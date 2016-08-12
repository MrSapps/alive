#include <sstream>
#include <algorithm>
#include <fstream>
#include <iterator>
#include "logger.hpp"
#include "oddlib/stream.hpp"
#include "oddlib/exceptions.hpp"

namespace Oddlib
{
    Stream::Stream(std::vector<u8>&& data)
    {
        mSize = data.size();
        auto s = std::make_unique<std::stringstream>();
        std::copy(data.begin(), data.end(), std::ostream_iterator<unsigned char>(*s));
        mStream = std::move(s);
        Seek(0);
        mName = "Memory buffer (" + std::to_string(mSize) + ") bytes";
        mFromMemoryBuffer = true;
    }

    Stream::Stream(const std::string& fileName)
        : mName(fileName)
    {
        auto s = std::make_unique<std::ifstream>();
        s->open(fileName, std::ios::in | std::ios::binary | std::ios::ate);
        if (!*s)
        {
            LOG_ERROR("File not found " << fileName);
            throw Exception("File not found");
        }

        mSize = static_cast<size_t>(s->tellg());
        s->seekg(std::ios::beg);

        mStream = std::move(s);
    }

    IStream* Stream::Clone()
    {
        if (!mFromMemoryBuffer)
        {
            return new Stream(mName);
        }
        auto oldPos = Pos();

        Seek(0);
        std::vector<u8> streamData(Size());
        ReadBytes(streamData.data(), Size());
        Seek(oldPos);

        return new Stream(std::move(streamData));
    }

    IStream* Stream::Clone(u32 /*start*/, u32 /*size*/)
    {
        throw Exception("Sub clone not supported on direct file streams");
    }

    template<typename T>
    void DoRead(std::unique_ptr<std::istream>& stream, T& output)
    {
        if (!stream->read(reinterpret_cast<char*>(&output), sizeof(output)))
        {
            throw Exception("Read failure");
        }
    }

    void Stream::ReadBytes(u8* pDest, size_t destSize)
    {
        if (!mStream->read(reinterpret_cast<char*>(pDest), destSize))
        {
            throw Exception("ReadBytes failure");
        }
    }

    void Stream::Seek(size_t pos)
    {
        if (!mStream->seekg(pos))
        {
            throw Exception("Seek failure");
        }
    }

    bool Stream::AtEnd() const
    {
        const int c = mStream->peek();
        return (c == EOF);
    }

    std::string Stream::LoadAllToString()
    {
        Seek(0);
        std::string content
        { 
            std::istreambuf_iterator<char>(*mStream),
            std::istreambuf_iterator<char>()
        };
        return content;
    }

    size_t Stream::Pos() const
    {
        const size_t pos = static_cast<size_t>(mStream->tellg());
        return pos;
    }

    size_t Stream::Size() const
    {
        return mSize;
    }
}
