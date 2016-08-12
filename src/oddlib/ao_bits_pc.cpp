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
        std::vector<u8> buffer;
       
        const u32 kStripSize = 16;
        const u32 kNumStrips = 640 / kStripSize;

        for (u32 i = 0; i < kNumStrips; i++)
        {
            // Read the size of the image strip
            u16 stripSize = 0;
            stream.Read(stripSize);

            // Read the raw image bytes
            buffer.resize(stripSize);
            stream.Read(buffer);

            // Convert to an SDL surface
            SDL_SurfacePtr strip(SDL_CreateRGBSurfaceFrom(buffer.data(), 16, 240, 16, kStripSize*sizeof(u16), red_mask, green_mask, blue_mask, 0));

            // Copy paste the strip into the full image
            SDL_Rect dstRect;
            dstRect.x = kStripSize * i;
            dstRect.y = 0;
            dstRect.w = kStripSize;
            dstRect.h = 240;
            SDL_BlitSurface(strip.get(), NULL, mSurface.get(), &dstRect);
        } 
        if (mSurface->format->format != SDL_PIXELFORMAT_RGB24)
        {
            mSurface.reset(SDL_ConvertSurfaceFormat(mSurface.get(), SDL_PIXELFORMAT_RGB24, 0));
        }
    }

}
