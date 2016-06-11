#pragma once

#include "sdl_raii.hpp"

namespace
{
    struct hsv
    {
        double h;       // angle in degrees
        double s;       // percent
        double v;       // percent
    };

    struct rgb
    {
        double r;       // angle in degrees
        double g;       // percent
        double b;       // percent
    };

    // Stolen from http://stackoverflow.com/questions/3018313/algorithm-to-convert-rgb-to-hsv-and-hsv-to-rgb-in-range-0-255-for-both
    inline hsv rgb2hsv(rgb in)
    {
        hsv         out = {};
        double      min, max, delta;

        min = in.r < in.g ? in.r : in.g;
        min = min  < in.b ? min : in.b;

        max = in.r > in.g ? in.r : in.g;
        max = max > in.b ? max : in.b;

        out.v = max;                                // v
        delta = max - min;
        if (delta < 0.00001)
        {
            out.s = 0;
            out.h = 0; // undefined, maybe nan?
            return out;
        }
        if (max > 0.0)
        {
            // NOTE: if Max is == 0, this divide would cause a crash
            out.s = (delta / max);                  // s
        }
        else
        {
            // if max is 0, then r = g = b = 0              
            // s = 0, v is undefined
            out.s = 0.0;
            out.h = NAN;                            // its now undefined
            return out;
        }
        if (in.r >= max)                           // > is bogus, just keeps compilor happy
        {
            out.h = (in.g - in.b) / delta;        // between yellow & magenta
        }
        else
        {
            if (in.g >= max)
            {
                out.h = 2.0 + (in.b - in.r) / delta;  // between cyan & yellow
            }
            else
            {
                out.h = 4.0 + (in.r - in.g) / delta;  // between magenta & cyan
            }
        }

        out.h *= 60.0;                              // degrees

        if (out.h < 0.0)
        {
            out.h += 360.0;
        }

        return out;
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
            //intensity[x][y] = rgb2hsv(resizedImage->pixels);
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
                        * cos(M_PI / 32.0 * (x + .5) * u)
                        * cos(M_PI / 32.0 * (y + .5) * v);
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

Uint32 hamming_distance(Uint64 hash1, Uint64 hash2)
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
