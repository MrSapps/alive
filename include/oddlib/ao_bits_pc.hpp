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
    private:
        void GenerateImage(IStream& stream);
        SDL_Surface* mSurface = nullptr;
    };
}