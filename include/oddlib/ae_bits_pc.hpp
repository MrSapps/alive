#pragma once

#include "sdl_raii.hpp"
#include "oddlib/bits_factory.hpp"
#include <vector>

namespace Oddlib
{
    class IStream;
    struct BitsLogic;

    class AeBitsPc : public IBits
    {
    public:
        AeBitsPc(const AeBitsPc&) = delete;
        AeBitsPc& operator = (const AeBitsPc&) = delete;
        explicit AeBitsPc(IStream& stream);
        virtual SDL_Surface* GetSurface() const override;
    private:
        void vlc_decode(const std::vector<Uint16>& aCamSeg, std::vector<Uint16>& aDst);
        void process_segment(Uint16* aVlcBufferPtr, int xPos);
        int next_bits();
        void vlc_decoder(int aR, int aG, int aB, signed int aWidth, int aVramX, int aVramY);
        void write_4_pixel_block(const BitsLogic& aR, const BitsLogic& aG, const BitsLogic& aB, int aVramX, int aVramY);

        signed int g_left7_array = 0;
        unsigned short int* g_pointer_to_vlc_buffer = nullptr;
        int g_right25_array = 0;
        unsigned short int g_vram[240][640];

        friend struct BitsLogic;

        void GenerateImage(IStream& stream);
        SDL_SurfacePtr mSurface;
    };
}
