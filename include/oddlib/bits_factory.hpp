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
        virtual ~IFg1();
        virtual SDL_Surface* GetSurface() const = 0;

        void Save(const std::string& baseName);
    };

    class IBits
    {
    public:
        virtual ~IBits();
        virtual SDL_Surface* GetSurface() const = 0;
        virtual IFg1* GetFg1() const = 0;
        void Save();
    };

    bool IsPsxCamera(IStream& stream);
    std::unique_ptr<IBits> MakeBits(SDL_SurfacePtr camImage);
    std::unique_ptr<IBits> MakeBits(IStream& bitsStream, IStream* fg1Stream);
}
