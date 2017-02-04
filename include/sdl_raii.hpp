#pragma once

#include "SDL.h"
#include <memory>
#include <fstream>
#include "logger.hpp"
#include "oddlib/exceptions.hpp"
#include "oddlib/stream.hpp"

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

    static void SaveSurfaceAsPng(const char* fileName, SDL_Surface* surface);

    static SDL_SurfacePtr LoadPng(Oddlib::IStream& stream, bool hasAlpha);

};
