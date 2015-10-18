#include "oddlib/psx_bits.hpp"
#include "oddlib/stream.hpp"
#include "logger.hpp"

namespace Oddlib
{
    PsxBits::PsxBits(IStream& stream)
    {
        GenerateImage(stream);
    }

    SDL_Surface* PsxBits::GetSurface() const
    {
        return mSurface.get();
    }

    void PsxBits::GenerateImage(IStream& stream)
    {
        const Uint32 kStripSize = 16;
        const Uint32 kNumStrips = 640 / kStripSize;

    
        for (Uint32 i = 0; i < kNumStrips; i++)
        {
            // Read the size of the image strip
            Uint16 stripSize = 0;
            stream.ReadUInt16(stripSize);

            if (stripSize > 0)
            {
                std::vector<Uint8> buffer(stripSize);
                stream.ReadBytes(buffer.data(), buffer.size());
            }
        }


    //    mSurface.reset(SDL_CreateRGBSurfaceFrom(g_vram, 640, 240, 16, 640 * sizeof(Uint16), red_mask, green_mask, blue_mask, 0));

        //SDL_SaveBMP(mSurface.get(), "testing.bmp");
    }
}
