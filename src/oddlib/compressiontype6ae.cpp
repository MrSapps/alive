#include "oddlib/compressiontype6ae.hpp"
#include "oddlib/stream.hpp"
#include "logger.hpp"
#include <vector>
#include <cassert>

namespace Oddlib
{

    // Function 0x0040A8A0 in AE
    std::vector<Uint8> CompressionType6Ae::Decompress(IStream& stream, Uint32 finalW, Uint32 w, Uint32 h, Uint32 /*dataSize*/)
    {
        std::vector<Uint8> out(finalW*h);

        Uint32 srcByte; // edx@2

        Sint32 byteCounter; // eax@3
        Sint32 bReadLowNibble; // ebp@5
        Sint32 bits_and_byte_count; // eax@7
        Sint32 cnt; // ecx@7
        Sint32 bitCount; // eax@16

        union intUni
        {
            Uint8 b[4];
            int i;
        } dstByte;

        Sint32 heightCounter; // [sp+Ch] [bp-8h]@2
        Uint8 srcByteBits; // [sp+1Ch] [bp+8h]@5
        Uint8 srcByteBits2; // [sp+1Ch] [bp+8h]@13

        const Sint32 width = w;
        Sint32 bReadLow = 0;
        const Sint32 height = h;
        Sint32 bBitsWriten = 0;
        
        Uint32 dstPos = 0;

        if (height > 0)
        {
            srcByte = 0;
            heightCounter = height;
            do
            {
                byteCounter = 0;
                while (byteCounter < width)
                {

                    if (bReadLow)
                    {
                        srcByteBits = static_cast<Uint8>(srcByte >> 4);
                        bReadLowNibble = 0;
                    }
                    else
                    {
                        bReadLowNibble = 1;
                        srcByte = ReadUInt8(stream);
                        srcByteBits = srcByte & 0xF;
                    }

                    cnt = srcByteBits;
                    bits_and_byte_count = srcByteBits + byteCounter;
                    if (srcByteBits > 0)
                    {
                        do
                        {
                            if (bBitsWriten)
                            {
                                dstPos++;
                                bBitsWriten = 0;
                            }
                            else
                            {
                                out[dstPos] = 0;
                                bBitsWriten = 1;
                            }
                            --cnt;
                        } while (cnt);
                    }

                    if (bReadLowNibble)
                    {
                        srcByteBits2 = static_cast<Uint8>(srcByte >> 4);
                        bReadLow = 0;
                    }
                    else
                    {
                        bReadLow = 1;
                        srcByte = ReadUInt8(stream);
                        srcByteBits2 = srcByte & 0xF;
                    }

                    byteCounter = srcByteBits2 + bits_and_byte_count;
                    if (srcByteBits2 > 0)
                    {
                        bitCount = srcByteBits2;
                        do
                        {
                            if (bReadLow)
                            {
                                dstByte.i = srcByte >> 4;
                                bReadLow = 0;
                            }
                            else
                            {
                                bReadLow = 1;
                                srcByte = ReadUInt8(stream);
                                dstByte.b[0] = static_cast<Uint8>(srcByte & 0xF);
                            }

                            if (bBitsWriten)
                            {
                                out[dstPos++] |= 16 * (Uint8)dstByte.i;
                                bBitsWriten = 0;
                            }
                            else
                            {
                                out[dstPos] = static_cast<Uint8>(dstByte.i);
                                bBitsWriten = 1;
                            }
                        } while (--bitCount);
                    }
                }

                for (; byteCounter & 7; ++byteCounter)
                {
                    if (bBitsWriten)
                    {
                        dstPos++;
                        bBitsWriten = 0;
                    }
                    else
                    {
                        out[dstPos] = 0;
                        bBitsWriten = 1;
                    }
                }

            } while (heightCounter-- != 1);
        }
        return out;
    }
}
