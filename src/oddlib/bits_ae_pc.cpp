#include "oddlib/ae_bits_pc.hpp"
#include "oddlib/stream.hpp"
#include "oddlib/vlctable.hpp"
#include "logger.hpp"

namespace Oddlib
{
    // NOTE: More reversing is required to fully understand these algorithms, but its something like JPEG
    // and since its enough to actually decode the data this is where the work on reversing stopped :)
    static const unsigned short int g_red_table[] =
    {
        0x00000, 0x00800, 0x01000, 0x01800, 0x02000, 0x02800, // 0
        0x03000, 0x03800, 0x04000, 0x04800, 0x05000, 0x05800, // 6
        0x06000, 0x06800, 0x07000, 0x07800, 0x08000, 0x08800, // 12
        0x09000, 0x09800, 0x0A000, 0x0A800, 0x0B000, 0x0B800, // 18
        0x0C000, 0x0C800, 0x0D000, 0x0D800, 0x0E000, 0x0E800, // 24
        0x0F000, 0x0F800, 0x0F800, 0x0F800, 0x0F800, 0x0F800, // 30
        0x0F800, 0x0F800, 0x0F800, 0x0F800, 0x0F800, 0x0F800, // 36
        0x0F800, 0x0F800, 0x0F800, 0x0F800, 0x0F800, 0x0F800, // 42
        0x0F800, 0x0F800, 0x0F800, 0x0F800, 0x0F800, 0x0F800, // 48
        0x0F800, 0x0F800, 0x0F800, 0x0F800, 0x0F800, 0x0F800, // 54
        0x0F800, 0x0F800, 0x0F800, 0x0F800                    // 60-64
    };

    static const unsigned short int g_blue_table[] =
    {
        0, 1, 2, 3, 4, 5,                   // 0
        6, 7, 8, 9, 0x0A, 0x0B,             // 6
        0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, // 12
        0x12, 0x13, 0x14, 0x15, 0x16, 0x17, // 18
        0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, // 24
        0x1E, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, // 30
        0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, // 36
        0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, // 42
        0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, // 48
        0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, // 54
        0x1F, 0x1F, 0x1F, 0x1F              // 60
    };

    static const unsigned short int g_green_table[] =
    {
        0x000, 0x040, 0x080, 0x0C0, 0x100, 0x140, // 0
        0x180, 0x1C0, 0x200, 0x240, 0x280, 0x2C0, // 6
        0x300, 0x340, 0x380, 0x3C0, 0x400, 0x440, // 12
        0x480, 0x4C0, 0x500, 0x540, 0x580, 0x5C0, // 18
        0x600, 0x640, 0x680, 0x6C0, 0x700, 0x740, // 24
        0x780, 0x7C0, 0x7C0, 0x7C0, 0x7C0, 0x7C0, // 30
        0x7C0, 0x7C0, 0x7C0, 0x7C0, 0x7C0, 0x7C0, // 36
        0x7C0, 0x7C0, 0x7C0, 0x7C0, 0x7C0, 0x7C0, // 42
        0x7C0, 0x7C0, 0x7C0, 0x7C0, 0x7C0, 0x7C0, // 48
        0x7C0, 0x7C0, 0x7C0, 0x7C0, 0x7C0, 0x7C0, // 54
        0x7C0, 0x7C0, 0x7C0, 0x7C0, 0x000         // 60
    };

    // Encapsulates the logic of vlc_decoder() each call can read 3 words or 6 bytes max
    struct BitsLogic
    {
        BitsLogic()
            : param1(0), param2(0), param3(0), param4(0)
        {

        }

        BitsLogic(int& aPrev, AeBitsPc* aStrat)
            : param1(0), param2(0), param3(0), param4(0)
        {
            // Grab 3x next bits
            bits[0] = aStrat->next_bits();
            bits[1] = aStrat->next_bits();
            bits[2] = aStrat->next_bits();

            // Round 1
            int calc1 = bits[2] - (bits[0] >> 1);
            int calc2 = calc1 + bits[0];
            int calc3 = aPrev - (bits[1] >> 1);

            // Round 2
            param1 = calc3 - (calc1 >> 1);
            param2 = param1 + calc1; // E.g calc1 = param2-param1, so calc3 = param1 + (calc1 >> 1) ?
            param3 = calc3 - (calc2 >> 1) + bits[1]; // knowing calc3 and 2 should yield bits[1]?

            // Final magic thats only used outside of the loop for pixel 4
            param4 = param3 + calc2; //calc2 = param4 - param3 ?

            aPrev = param3 + calc2; // how we calc bottom right
        }

        // Read from the cam file data
        int bits[3]; // Used outside

        int param1;
        int param2;
        int param3;

        // Only used out of the loop
        int param4;
    };


    const auto red_mask = 0xF800;
    const auto green_mask = 0x7E0;
    const auto blue_mask = 0x1F;

    AeBitsPc::AeBitsPc(IStream& stream)
    {
        GenerateImage(stream);
    }

    SDL_Surface* AeBitsPc::GetSurface() const
    {
        return mSurface.get();
    }

    void AeBitsPc::GenerateImage(IStream& stream)
    {
        const u32 kStripSize = 16;
        const u32 kNumStrips = 640 / kStripSize;

        memset(g_vram, 0, sizeof(g_vram));

        for (u32 i = 0; i < kNumStrips; i++)
        {
            // Read the size of the image strip
            u16 stripSize = 0;
            stream.Read(stripSize);

            if (stripSize > 0)
            {
                // Raw segment from the CAM bits chunk
                std::vector<u16> rawBitsFromFile;
                rawBitsFromFile.resize(stripSize / sizeof(u16));
                stream.Read(rawBitsFromFile);
                
                // The vlc_decode reads futher that what we read from the file, hence keep double size!
                rawBitsFromFile.resize(stripSize);

                // Create a "VLC" buffer to store the decompressed data in.
                std::vector<u16> vlcBuf;
                vlcBuf.resize(0x7E00); // 4KB

                // Decompress the segment into the vlc buffer
                vlc_decode(rawBitsFromFile, vlcBuf);

                // write out the decoded pixels
                process_segment(vlcBuf.data(), i * 16);
            }
        }

        mSurface.reset(SDL_CreateRGBSurfaceFrom(g_vram, 640, 240, 16, 640 * sizeof(u16), red_mask, green_mask, blue_mask, 0));
        if (mSurface->format->format != SDL_PIXELFORMAT_RGB24)
        {
            mSurface.reset(SDL_ConvertSurfaceFormat(mSurface.get(), SDL_PIXELFORMAT_RGB24, 0));
        }
    }

    void AeBitsPc::vlc_decode(const std::vector<u16>& aCamSeg, std::vector<u16>& aDst)
    {
        unsigned int vlcPtrIndex = 0;
        unsigned int camSrcPtrIndex = 0;
        unsigned int vlcTabIndex = 0;

        // Or two source words together to make a DWORD
        unsigned int dstVlcWord = aCamSeg[camSrcPtrIndex + 1] | (aCamSeg[camSrcPtrIndex] << 16);
        camSrcPtrIndex += 2; // Skip the two words we just OR'ed

        signed int totalBitsToShiftBy = 0;

        for (;;)
        {
            // Get 11 bits
            const unsigned int shiftedData = (dstVlcWord >> 21);

            // 0b11111111111 = 2047 * 4 =8192 = 8kb max index of 11 bits
            vlcTabIndex = 4 * shiftedData;

            // Grab vlc tab short using 11bit index * 4
            const unsigned int bitsToShiftBy = g_VlcTab[vlcTabIndex];

            // Shift var
            totalBitsToShiftBy += bitsToShiftBy;

            // Shift the other way by the vlc word
            dstVlcWord = dstVlcWord << bitsToShiftBy;

            // If we've shifted more than sizeof(short)
            if (totalBitsToShiftBy > 0xF)
            {
                // Limit to short, and set 1st short to the next
                // source word
                totalBitsToShiftBy = totalBitsToShiftBy & 0xF;
                dstVlcWord |= aCamSeg[camSrcPtrIndex++] << totalBitsToShiftBy;
            }

            int counter = 4;
            while (--counter)
            {
                unsigned short vlcWord = g_VlcTab[++vlcTabIndex];
                if (vlcWord == 0)
                {
                    counter = 0; // continue
                    break;
                }
                else if (vlcWord == 0xFFFF) // 1. Get an index that results in >0xf before here and ends up in here.
                {
                    counter = 1; // don't continue
                    break;
                }
                else
                {
                    // Copy next table word
                    aDst[vlcPtrIndex++] = vlcWord;
                }
            }
            if (counter == 0)
            {
                ++vlcTabIndex;
                continue;
            }

            // 2. Then get a none "1" value here and we can just take the 13bits to get
            // whatever value we want? Would have to assume we always want 0-0x1fff
            // values!

            // This can happen a lot (i.e this exit path is not taken once)
            // 0x1fff = 0b1111111111111 = max of >> 19 (13bits)
            aDst[vlcPtrIndex++] = dstVlcWord >> 19; // Thus the last word is always 1?

            if (dstVlcWord >> 19 == 1) // If bit 1 of the shifted 13bits is enabled only (could be &1 == 1)
            {
                // Exit function!

                // TODO: Can resize vector at this point..?

                break; // or return
            }
            totalBitsToShiftBy += 0xD;
            dstVlcWord = dstVlcWord << 0xD; // 0xD = 13 jib the bits above?

            if (totalBitsToShiftBy > 0xF) // Limit to 16 and pull another shifted byte in
            {
                totalBitsToShiftBy = totalBitsToShiftBy & 0xF; // Could just assign here?
                dstVlcWord |= aCamSeg[camSrcPtrIndex++] << totalBitsToShiftBy; // Move the word over if 0xF!
            }
        }
    }

    // This function takes a 16x240 strip of bits and processes as 16x16 sized macro blocks, thus there are 240/16=15 macro blocks
    void AeBitsPc::process_segment(u16 *aVlcBufferPtr, int xPos)
    {
        g_pointer_to_vlc_buffer = aVlcBufferPtr;       // This is decoding one 16x240 seg

        // Must be init to zero!
        g_left7_array = 0;
        next_bits();

        // 240/16 = 15 macro blocks for this strip
        for (int blockNo = 0; blockNo<16; blockNo++)
        {
            // Each 16x16 block is decoded using a quad tree breaking it up in to 64 2x2 blocks
            int notUsed = 0;

            BitsLogic logic(notUsed, this);
            vlc_decoder(logic.bits[0], logic.bits[1], logic.bits[2], 16, xPos, blockNo * 16); // 16 is the width/block size
        }
    }

    // Get 25 bits, keep looping until the next 7bits hits zero (use it as a counter)
    int AeBitsPc::next_bits()
    {
        int ret = 0;
        if (g_left7_array <= 0)
        {
            ret = g_right25_array; // Always the previous g_right25_array! Or zero on first/ when its RLE data

            unsigned short int o = *g_pointer_to_vlc_buffer;

            g_left7_array = o >> 7;

            // Get 25 bits
            g_right25_array = (o << 25) >> 25;

            // To next word
            ++g_pointer_to_vlc_buffer;
        }
        else
        {
            // This is RLE? I.e if g_left7_array is 7 then we repeat 7 pixels?
            --g_left7_array;
        }
        return ret;
    }

    void AeBitsPc::vlc_decoder(int aR, int aG, int aB, signed int aWidth, int aVramX, int aVramY)
    {
        while (aWidth != 2) // Quad tree through 16, 8, 4, 2 sizes
        {
            // These extra 3 are for each quad tree block? (of 16,8,4?) plus an extra 3 on the very first call?
            BitsLogic logic1(aR, this); // 1st param is a reference so call ordering matters
            BitsLogic logic2(aG, this);
            BitsLogic logic3(aB, this);

            aWidth = aWidth / 2;

            vlc_decoder(logic1.param1, logic2.param1, logic3.param1, aWidth, aVramX, aVramY);          // first block (top left)
            vlc_decoder(logic1.param2, logic2.param2, logic3.param2, aWidth, aVramX + aWidth, aVramY); // (top right)
            vlc_decoder(logic1.param3, logic2.param3, logic3.param3, aWidth, aVramX, aVramY + aWidth); // (bottom left)

            // last block (bottom right), never done until w=2 though?
            aVramY = aVramY + aWidth;
            aVramX = aVramX + aWidth;
        }

        BitsLogic r(aR, this);
        BitsLogic g(aG, this);
        BitsLogic b(aB, this);
        write_4_pixel_block(r, g, b, aVramX, aVramY);
    }



    void AeBitsPc::write_4_pixel_block(const BitsLogic& aR, const BitsLogic& aG, const BitsLogic& aB, int aVramX, int aVramY)
    {
        // BL
        if (aVramY < 240 && aVramX < 640)
        {
            g_vram[aVramY][aVramX] = g_red_table[aR.param1] | g_green_table[aG.param1] | g_blue_table[aB.param1];
        }

        // TR
        if (aVramY < 240 && aVramX+1 < 640)
        {
            g_vram[aVramY][aVramX + 1] = g_red_table[aR.param2] | g_green_table[aG.param2] | g_blue_table[aB.param2];
        }

        // BL
        if (aVramY+1 < 240 && aVramX < 640)
        {
            g_vram[aVramY + 1][aVramX] = g_red_table[aR.param3] | g_green_table[aG.param3] | g_blue_table[aB.param3];
        }

        // BR
        if (aVramY+1 < 240 && aVramX+1 < 640)
        {
            g_vram[aVramY + 1][aVramX + 1] = g_red_table[aR.param4] | g_green_table[aG.param4] | g_blue_table[aB.param4];
        }
    }

}
