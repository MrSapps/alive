#pragma once

#include "SDL.h"
#include <memory>
#include <string>
#include "oddlib/lvlarchive.hpp"

namespace Oddlib
{
    class IStream;

    class IBits
    {
    public:
        IBits(std::shared_ptr<Oddlib::LvlArchive>& lvl)
            : mLvl(lvl)
        {

        }

        virtual ~IBits() = default;
        virtual SDL_Surface* GetSurface() const = 0;

        void Save()
        {
            static int i = 1;
            SDL_SaveBMP(GetSurface(), ("camera" + std::to_string(i++) + ".bmp").c_str());
        }
    private:
        std::shared_ptr<Oddlib::LvlArchive> mLvl;
    };

    bool IsPsxCamera(IStream& stream);
    std::unique_ptr<IBits> MakeBits(IStream& stream, std::shared_ptr<Oddlib::LvlArchive>& lvl);
}
