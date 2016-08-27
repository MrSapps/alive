#pragma once

#include <vector>
#include "types.hpp"

namespace Oddlib
{
    class IStream;
    template<u32 BitsSize>
    class CompressionType6or7AePsx
    {
    public:
        CompressionType6or7AePsx() = default;
        std::vector<u8> Decompress(IStream& stream, u32 finalW, u32 w, u32 h, u32 dataSize);
    };
}
