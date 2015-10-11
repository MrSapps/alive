#pragma once

#include "SDL.h"

namespace Oddlib
{
    class IStream;

    class AoBitsPc
    {
    public:
        AoBitsPc(const AoBitsPc&) = delete;
        AoBitsPc& operator = (const AoBitsPc&) = delete;
        explicit AoBitsPc(IStream& stream);
        ~AoBitsPc();

        void *getPixelData() const { return mSurface->pixels; }
        int getImageWidth() const { return mSurface->w; }
        int getImageHeight() const { return mSurface->h; }
        SDL_PixelFormat *getImageFormat() const { return mSurface->format; }

    private:
        void GenerateImage(IStream& stream);
        SDL_Surface* mSurface = nullptr;
    };
}