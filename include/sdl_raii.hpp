#pragma once

#include "SDL.h"
#include <memory>

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
