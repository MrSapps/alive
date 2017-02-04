#include "oddlib/bits_fg1_ae_pc.hpp"
#include "bitutils.hpp"

namespace Oddlib
{
    enum eChunkTypes
    {
        ePartialChunk = 0,
        eFullChunk = 0xFFFE,
        eEndChunk = 0xFFFF
    };

    struct FgChunk
    {
        u16 nType;

        // TODO: Need to collate ever used value here and check what they really mean in terms of draw order
        u16 nLayer;

        // Where to draw this block in the FG.
        u16 nXOffset;
        u16 nYOffset;

        // Width and height of a block. Max is 32x16.
        u16 nWidth;
        u16 nHeight;

        void Read(IStream& stream)
        {
            stream.Read(nType);
            stream.Read(nLayer);
            stream.Read(nXOffset);
            stream.Read(nYOffset);
            stream.Read(nWidth);
            stream.Read(nHeight);
        }
    };

    static void set_pixel_alpha(SDL_Surface *surface, int x, int y, Uint8 alpha)
    {
        Uint32 *target_pixel = (Uint32 *)((Uint8 *)surface->pixels + y * surface->pitch + x * sizeof *target_pixel);
        *target_pixel |= ((((Uint32)alpha) << 24));
    }

    BitsFg1AePc::BitsFg1AePc(SDL_Surface* camera, IStream& stream)
    {
        TRACE_ENTRYEXIT;

        // TODO: Common masks
        SDL_SurfacePtr fg1(SDL_CreateRGBSurface(0, camera->w, camera->h, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0));

        u32 numberOfPartialChunks = 0;
        stream.Read(numberOfPartialChunks);

        // Duplicate ALL of the source - then turn of the alpha for the bits we want to show
        // if we don't copy all of it then texture filtering will blend the bits with copied with 
        // black pixels and cause strange rendering artifacts.
        SDL_BlitSurface(camera, NULL, fg1.get(), NULL);

        FgChunk chunk = {};
        u32 chunksRead = 0;
        for (;;)
        {
            chunk.Read(stream);

            if (chunk.nType == ePartialChunk)
            {
                chunksRead++;

                LOG_INFO("ePartialBlock");
                if (chunk.nWidth && chunk.nHeight)
                {
                    for (u32 y = 0; y < chunk.nHeight; y++)
                    {
                        u32 bitMask = 0;
                        stream.Read(bitMask);
                        for (u32 x = 0; x < chunk.nWidth; x++)
                        {
                            if (IsBitOn(bitMask, x))
                            {
                                set_pixel_alpha(fg1.get(), x + chunk.nXOffset, y + chunk.nYOffset, 255);
                            }
                        }
                    }
                }
            }
            else if (chunk.nType == eFullChunk)
            {
                LOG_INFO("eFullBlock");
                for (u32 x = 0; x < chunk.nWidth; x++)
                {
                    for (u32 y = 0; y < chunk.nHeight; y++)
                    {
                        set_pixel_alpha(fg1.get(), x + chunk.nXOffset, y + chunk.nYOffset, 255);
                    }
                }
            }
            else if (chunk.nType == eEndChunk)
            {
                if (chunksRead != numberOfPartialChunks)
                {
                    LOG_ERROR("End block hit before all chunks read");
                }
                break;
            }
            else
            {
                LOG_ERROR("Unknown block type: " << chunk.nType);
                abort();
            }
        }

        // Exited the loop with out reading an EOF block
        if (chunk.nType != eEndChunk)
        {
            LOG_ERROR("End chunk is missing");
        }

        mSurface = std::move(fg1);
    }

    SDL_Surface* BitsFg1AePc::GetSurface() const
    {
        return mSurface.get();
    }

}
