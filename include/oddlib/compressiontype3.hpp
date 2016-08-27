#pragma once

#include <vector>
#include "types.hpp"

namespace Oddlib
{
    class IStream;
    class CompressionType3
    {
    public:
        CompressionType3() = default;
        std::vector<u8> Decompress(IStream& stream, u32 finalW, u32 w, u32 h, u32 dataSize);
    };
}
