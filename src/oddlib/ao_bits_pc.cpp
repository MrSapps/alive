#include "oddlib/ao_bits_pc.hpp"
#include "oddlib/stream.hpp"

namespace Oddlib
{
    const auto red_mask = 0xF800;
    const auto green_mask = 0x7E0;
    const auto blue_mask = 0x1F;

    AoBitsPc::AoBitsPc(IStream& stream)
    {
        mSurface.reset(SDL_CreateRGBSurface(0, 640, 240, 16, red_mask, green_mask, blue_mask, 0));
        GenerateImage(stream);
    }

    SDL_Surface* AoBitsPc::GetSurface() const
    {
        return mSurface.get();
    }

    void AoBitsPc::GenerateImage(IStream& stream)
    {
        std::vector<Uint8> buffer;
       
        const Uint32 kStripSize = 16;
        const Uint32 kNumStrips = 640 / kStripSize;

        for (Uint32 i = 0; i < kNumStrips; i++)
        {
            // Read the size of the image strip
            Uint16 stripSize = 0;
            stream.ReadUInt16(stripSize);

            // Read the raw image bytes
            buffer.resize(stripSize);
            stream.ReadBytes(buffer.data(), buffer.size());

            // Convert to an SDL surface
            SDL_SurfacePtr strip(SDL_CreateRGBSurfaceFrom(buffer.data(), 16, 240, 16, kStripSize*sizeof(Uint16), red_mask, green_mask, blue_mask, 0));

            // Copy paste the strip into the full image
            SDL_Rect dstRect;
            dstRect.x = kStripSize * i;
            dstRect.y = 0;
            dstRect.w = kStripSize;
            dstRect.h = 240;
            SDL_BlitSurface(strip.get(), NULL, mSurface.get(), &dstRect);
        }

        //SDL_SaveBMP(mSurface.get(), "testing.bmp");
    }

}
