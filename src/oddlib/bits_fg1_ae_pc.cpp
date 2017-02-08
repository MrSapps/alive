#include "oddlib/bits_fg1_ae_pc.hpp"
#include "oddlib/compressiontype4or5.hpp"
#include "bitutils.hpp"

namespace Oddlib
{
    enum eChunkTypes
    {
        ePartialChunk = 0,
        eFullChunk = 0xFFFE,
        eEndCompressedData = 0xFFFC,     // Ao only
        eStartCompressedData = 0xFFFD,   // Ao only
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

    static void ProcessFG1(SDL_Surface* fg1, IStream& stream, u32 numberOfPartialChunks, u32& chunksRead, bool bBitMaskedPartialBlocks)
    {
        FgChunk chunk = {};
        for (;;)
        {
            chunk.Read(stream);

            if (chunk.nType == ePartialChunk)
            {
                chunksRead++;

                LOG_INFO("ePartialBlock");
                if (chunk.nWidth && chunk.nHeight)
                {
                    if (bBitMaskedPartialBlocks)
                    {
                        for (u32 y = 0; y < chunk.nHeight; y++)
                        {

                            u32 bitMask = 0;
                            stream.Read(bitMask);
                            for (u32 x = 0; x < chunk.nWidth; x++)
                            {
                                if (IsBitOn(bitMask, x))
                                {
                                    set_pixel_alpha(fg1, x + chunk.nXOffset, y + chunk.nYOffset, 255);
                                }
                            }
                        }
                    }
                    else
                    {
                        for (u32 y = 0; y < chunk.nHeight; y++)
                        {
                            for (u32 x = 0; x < chunk.nWidth; x++)
                            {
                                u16 pixel = 0;
                                stream.Read(pixel);
                                if (pixel)
                                {
                                    set_pixel_alpha(fg1, x + chunk.nXOffset, y + chunk.nYOffset, 255);
                                }
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
                        set_pixel_alpha(fg1, x + chunk.nXOffset, y + chunk.nYOffset, 255);
                    }
                }
            }
            else if (chunk.nType == eStartCompressedData)
            {
                const size_t oldPos = stream.Pos();

                u32 uncompressedSize = 0;
                stream.Read(uncompressedSize);

                CompressionType4Or5 dec;
                auto data = dec.Decompress(stream, 0, 0, 0, 0); // TODO: Reads the wrong amount of data most of the time ??

                MemoryStream ms(std::move(data));
                ProcessFG1(fg1, ms, numberOfPartialChunks, chunksRead, bBitMaskedPartialBlocks);

                stream.Seek(oldPos + chunk.nXOffset);
            }
            else if (chunk.nType == eEndCompressedData)
            {
                return;
            }
            else if (chunk.nType == eEndChunk)
            {
                if (chunksRead != numberOfPartialChunks)
                {
                    LOG_ERROR("End block hit before all chunks read");
                }
                return;
            }
            else
            {
                LOG_ERROR("Unknown block type: " << chunk.nType);
                abort();
            }
        }
    }

    BitsFg1AePc::BitsFg1AePc(SDL_Surface* camera, IStream& stream, bool bBitMaskedPartialBlocks)
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

        u32 chunksRead = 0;
        ProcessFG1(fg1.get(), stream, numberOfPartialChunks, chunksRead, bBitMaskedPartialBlocks);
      
        mSurface = std::move(fg1);
    }

    SDL_Surface* BitsFg1AePc::GetSurface() const
    {
        return mSurface.get();
    }

}
