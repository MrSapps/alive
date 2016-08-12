#pragma once

#include "sdl_raii.hpp"
#include <cmath>

namespace
{
    inline u32 get_pixel32(SDL_Surface *surface, int x, int y)
    {
        //Convert the pixels to 32 bit
        u32 *pixels = (u32 *)surface->pixels;

        //Get the requested pixel
        return pixels[(y * surface->w) + x];
    }
}

// Mostly based on https://github.com/Cairnarvon/phash-dups/blob/master/src/phash.c
inline Uint64 pHash(SDL_Surface* image)
{
    // Resize to 32x32
    SDL_Rect resizeRect = { 0, 0, 32, 32 };
    SDL_SurfacePtr resizedImage = SDLHelpers::ScaledCopy(image, &resizeRect);

    // Convert from RGBA to HSV
    double intensity[32][32] = {};
    for (int x = 0; x < 32; x++)
    {
        for (int y = 0; y < 32; y++)
        {
            u32 pixel = get_pixel32(resizedImage.get(), x, y);

            double tmp = static_cast<double>(pixel);
            tmp = tmp / 4294967295;
            intensity[x][y] = tmp;
        }
    }

    // Discrete cosine transform
    double seq[64] = {};
    unsigned i = 0;
    for (int u = 0; u < 8; ++u) 
    {
        for (int v = 0; v < 8; ++v)
        {
            double acc = 0.0;
            for (int x = 0; x < 32; ++x) 
            {
                for (int y = 0; y < 32; ++y)
                {
                    acc += intensity[x][y]
                        * std::cos(M_PI / 32.0 * (x + .5) * u)
                        * std::cos(M_PI / 32.0 * (y + .5) * v);
                }
            }
            seq[i++] = acc;
        }
    }

    double avg = 0.0;
    for (i = 1; i < 63; ++i)
    {
        avg += seq[i];
    }

    avg /= 63;

    Uint64 retval = 0;
    for (i = 0; i < 64; ++i) 
    {
        uint64_t x = seq[i] > avg;
        retval |= x << i;
    }

    return retval;
}

inline u32 hamming_distance(Uint64 hash1, Uint64 hash2)
{
    unsigned ret = 0;
    Uint64 hashBits = hash1 ^ hash2;
    while (hashBits) 
    {
        ++ret;
        hashBits &= hashBits - 1;
    }
    return ret;
}
