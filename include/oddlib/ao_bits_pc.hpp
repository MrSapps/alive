#pragma once

#include "SDL.h"
#include "sdl_raii.hpp"

namespace Oddlib
{
    class IStream;

    class AoBitsPc
    {
    public:
        AoBitsPc(const AoBitsPc&) = delete;
        AoBitsPc& operator = (const AoBitsPc&) = delete;
        explicit AoBitsPc(IStream& stream);
    private:
        void GenerateImage(IStream& stream);
        SDL_SurfacePtr mSurface;
    };
}