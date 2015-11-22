#pragma once

#include "SDL.h"
#include <vector>

namespace Oddlib
{
    class IStream;
    template<Uint32 BitsSize>
    class CompressionType6or7AePsx
    {
    public:
        CompressionType6or7AePsx() = default;
        std::vector<Uint8> Decompress(IStream& stream, Uint32 finalW, Uint32 w, Uint32 h, Uint32 dataSize);
    };
}
