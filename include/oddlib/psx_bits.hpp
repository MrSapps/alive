#pragma once

#include "sdl_raii.hpp"
#include "oddlib/bits_factory.hpp"
#include <vector>

namespace Oddlib
{
    class IStream;
    struct BitsLogic;

    class PsxBits : public IBits
    {
    public:
        PsxBits(const PsxBits&) = delete;
        PsxBits& operator = (const PsxBits&) = delete;
        explicit PsxBits(IStream& stream);
        virtual SDL_Surface* GetSurface() const override;
    private:
        void GenerateImage(IStream& stream);
        SDL_SurfacePtr mSurface;
    };
}
