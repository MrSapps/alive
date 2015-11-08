#include "oddlib/compressiontype6ae.hpp"
#include "oddlib/stream.hpp"
#include "logger.hpp"
#include <vector>
#include <cassert>

namespace Oddlib
{
    static Uint8 ReadNibble(IStream& stream, bool readLo, Uint8& srcByte)
    {
        if (readLo)
        {
            return srcByte >> 4;
        }
        else
        {
            srcByte = ReadUInt8(stream);
            return srcByte & 0xF;
        }
    }

    // Function 0x0040A8A0 in AE
    std::vector<Uint8> CompressionType6Ae::Decompress(IStream& stream, Uint32 finalW, Uint32 w, Uint32 h, Uint32 /*dataSize*/)
    {
        std::vector<Uint8> out(finalW*h);

        Uint8 srcByte; // edx@2

        Uint32 byteCounter; // eax@3
        bool bNibbleToRead = false; // ebp@5
        Sint32 bits_and_byte_count; // eax@7
        Sint32 cnt; // ecx@7
        Sint32 bitCount; // eax@16

        union intUni
        {
            Uint8 b[4];
            int i;
        } dstByte;


        const Sint32 height = h;
        bool bBitsWriten = false;
        
        Uint32 dstPos = 0;

        if (height > 0)
        {
            srcByte = 0;
            Sint32 heightCounter = height;
            do
            {
                byteCounter = 0;
                while (byteCounter < w)
                {
                    Uint8 nibble = ReadNibble(stream, bNibbleToRead, srcByte);
                    bNibbleToRead = !bNibbleToRead;

                    cnt = nibble;
                    bits_and_byte_count = nibble + byteCounter;
                    if (nibble > 0)
                    {
                        do
                        {
                            if (bBitsWriten)
                            {
                                dstPos++;
                            }
                            else
                            {
                                out[dstPos] = 0;
                            }
                            bBitsWriten = !bBitsWriten;
                            --cnt;
                        } while (cnt);
                    }

                    nibble = ReadNibble(stream, bNibbleToRead, srcByte);
                    bNibbleToRead = !bNibbleToRead;

                    byteCounter = nibble + bits_and_byte_count;
                    if (nibble > 0)
                    {
                        bitCount = nibble;
                        do
                        {
                            dstByte.b[0] = ReadNibble(stream, bNibbleToRead, srcByte);
                            bNibbleToRead = !bNibbleToRead;


                            if (bBitsWriten)
                            {
                                out[dstPos++] |= 16 * dstByte.b[0];
                                bBitsWriten = 0;
                            }
                            else
                            {
                                out[dstPos] = dstByte.b[0];
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
                    }
                    else
                    {
                        out[dstPos] = 0;
                    }
                    bBitsWriten = !bBitsWriten;
                }

            } while (heightCounter-- != 1);
        }
        return out;
    }
}
