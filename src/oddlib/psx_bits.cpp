#include "oddlib/psx_bits.hpp"
#include "oddlib/stream.hpp"
#include "oddlib/PSXMDECDecoder.h"
#include "logger.hpp"
#include <string>

namespace Oddlib
{
    const auto red_mask = 0xF800;
    const auto green_mask = 0x7E0;
    const auto blue_mask = 0x1F;

    PsxBits::PsxBits(IStream& stream, bool includeLengthInStripSize, bool singleSlice)
        : mIncludeLengthInStripSize(includeLengthInStripSize)
    {
        mSurface.reset(SDL_CreateRGBSurface(0, 368, 240, 16, red_mask, green_mask, blue_mask, 0));
        GenerateImage(stream, singleSlice);
    }

    SDL_Surface* PsxBits::GetSurface() const
    {
        return mSurface.get();
    }

    void PsxBits::GenerateImage(IStream& stream, bool singleSlice)
    {

        u32 rmask, gmask, bmask, amask;

        /* SDL interprets each pixel as a 32-bit number, so our masks must depend
        on the endianness (byte order) of the machine */
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
        rmask = 0xff000000;
        gmask = 0x00ff0000;
        bmask = 0x0000ff00;
        amask = 0x000000ff;
#else
        rmask = 0x000000ff;
        gmask = 0x0000ff00;
        bmask = 0x00ff0000;
        amask = 0xff000000;
#endif


        int numSlices = (384 / 32);
        for (int u = 0; u < numSlices; u++)
        {
            PSXMDECDecoder mdec;

            u16 w = 0;
            if (singleSlice)
            {
                w = 368;
            }
            else
            {
                w = (u == numSlices - 1) ? 16 : 32;
            }

            SDL_SurfacePtr strip(SDL_CreateRGBSurface(0, w, 240, 32, rmask, gmask, bmask, amask));

            u16 len = 0;
            if (singleSlice)
            {
                len = static_cast<u16>(stream.Size());
            }
            else
            {
                stream.ReadUInt16(len);
                if (mIncludeLengthInStripSize)
                {
                    u16 dummy = 0;
                    stream.ReadUInt16(dummy);
                }
            }

            std::vector<u8> buffer(len);
            stream.ReadBytes(buffer.data(), buffer.size());
            buffer.resize(buffer.size()*2 );


            mdec.DecodeFrameToABGR32((uint16_t*)strip->pixels, (uint16_t*)(buffer.data()),w, 240);

            SDL_Rect dstRect = {};
            dstRect.x = 32 * u;
            dstRect.y = 0;
            dstRect.w = w;
            dstRect.h = 240;
            SDL_BlitSurface(strip.get(), NULL, mSurface.get(), &dstRect);

            if (singleSlice)
            {
                break;
            }
        }

        if (mSurface->format->format != SDL_PIXELFORMAT_RGB24)
        {
            mSurface.reset(SDL_ConvertSurfaceFormat(mSurface.get(), SDL_PIXELFORMAT_RGB24, 0));
        }
    }

}
