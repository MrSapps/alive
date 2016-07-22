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
        PsxBits(IStream& stream, bool includeLengthInStripSize, bool singleSlice);
        virtual SDL_Surface* GetSurface() const override;
        bool IncludeLength() const { return mIncludeLengthInStripSize; }
    private:
        void GenerateImage(IStream& stream, bool singleSlice);
        SDL_SurfacePtr mSurface;
        bool mIncludeLengthInStripSize = false;
    };
}
