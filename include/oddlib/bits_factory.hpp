#pragma once

#include "SDL.h"
#include <memory>

namespace Oddlib
{
    class IStream;

    class IBits
    {
    public:
        virtual ~IBits() = default;
        virtual SDL_Surface* GetSurface() const = 0;
    };

    std::unique_ptr<IBits> MakeBits(IStream& stream);
}
