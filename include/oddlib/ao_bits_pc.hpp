#pragma once

#include "SDL.h"
#include "sdl_raii.hpp"
#include "oddlib/bits_factory.hpp"

namespace Oddlib
{
    class IStream;

    class AoBitsPc : public IBits
    {
    public:
        AoBitsPc(const AoBitsPc&) = delete;
        AoBitsPc& operator = (const AoBitsPc&) = delete;
        explicit AoBitsPc(IStream& stream);

        // Returns observing pointer to surface
        virtual SDL_Surface* GetSurface() const override;
    private:
        void GenerateImage(IStream& stream);
        SDL_SurfacePtr mSurface;
    };
}