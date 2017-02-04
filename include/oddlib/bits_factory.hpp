#pragma once

#include "SDL.h"
#include <memory>
#include <string>
#include "oddlib/lvlarchive.hpp"
#include "sdl_raii.hpp"

namespace Oddlib
{
    class IStream;

    class IFg1
    {
    public:
        virtual ~IFg1() = default;
        virtual SDL_Surface* GetSurface() const = 0;

        void Save()
        {
            static int i = 1;
            SDL_SaveBMP(GetSurface(), ("camera_fg1" + std::to_string(i++) + ".bmp").c_str());
        }
    };

    class IBits
    {
    public:
        virtual ~IBits() = default;
        virtual SDL_Surface* GetSurface() const = 0;
        virtual IFg1* GetFg1() const = 0;

        void Save()
        {
            static int i = 1;
            SDL_SaveBMP(GetSurface(), ("camera" + std::to_string(i++) + ".bmp").c_str());
        }
    };

    bool IsPsxCamera(IStream& stream);
    std::unique_ptr<IBits> MakeBits(SDL_SurfacePtr camImage);
    std::unique_ptr<IBits> MakeBits(IStream& bitsStream, IStream* fg1Stream);
}
