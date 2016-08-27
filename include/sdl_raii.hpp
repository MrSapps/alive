#pragma once

#include "SDL.h"
#include <memory>
#include <fstream>
#include "lodepng/lodepng.h"
#include "oddlib/exceptions.hpp"

struct FreeSurface_Functor
{
    void operator() (SDL_Surface* pSurface) const
    {
        if (pSurface)
        {
            SDL_FreeSurface(pSurface);
        }
    }
};

typedef std::unique_ptr<SDL_Surface, FreeSurface_Functor> SDL_SurfacePtr;

class SDLHelpers
{
public:
    SDLHelpers() = delete;

    static SDL_SurfacePtr ScaledCopy(SDL_Surface* src, SDL_Rect* dstSize)
    {
        SDL_SurfacePtr scaledCopy(SDL_CreateRGBSurface(0, 
            dstSize->w, dstSize->h, 
            src->format->BitsPerPixel, 
            src->format->Rmask, src->format->Gmask, src->format->Bmask, src->format->Amask));

        // Get the old mode
        SDL_BlendMode oldBlendMode;
        SDL_GetSurfaceBlendMode(src, &oldBlendMode);

        // Set the new mode so copying the source won't change the source
        SDL_SetSurfaceBlendMode(src, SDL_BLENDMODE_NONE);

        // Do the copy
        if (SDL_BlitScaled(src, NULL, scaledCopy.get(), dstSize) != 0)
        {
            scaledCopy.reset();
        }

        // Restore the original blending mode
        SDL_SetSurfaceBlendMode(src, oldBlendMode);
        return scaledCopy;
    }

    static void SaveSurfaceAsPng(const char* fileName, SDL_Surface* surface)
    {
        lodepng::State state = {};

        // input color type
        state.info_raw.colortype = LCT_RGBA;
        state.info_raw.bitdepth = 8;

        // output color type
        state.info_png.color.colortype = LCT_RGBA;
        state.info_png.color.bitdepth = 8;
        state.encoder.auto_convert = 0;

        // encode to PNG
        std::vector<unsigned char> out;
        lodepng::encode(out, (const unsigned char*)surface->pixels, surface->w, surface->h, state);

        // write out PNG buffer
        std::ofstream fileStream;
        fileStream.open(fileName, std::ios::binary);
        if (!fileStream.is_open())
        {
            throw Oddlib::Exception("Can't open output file");
        }

        fileStream.write(reinterpret_cast<const char*>(out.data()), out.size());
    }
};
