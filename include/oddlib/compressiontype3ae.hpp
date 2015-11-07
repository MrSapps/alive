#pragma once

#include "SDL.h"
#include <vector>

namespace Oddlib
{
    class IStream;
    class CompressionType3Ae
    {
    public:
        CompressionType3Ae() = default;
        std::vector<Uint8> Decompress(IStream& stream, Uint32 size, Uint32 w, Uint32 h);
    };
}