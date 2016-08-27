#include "oddlib/compressiontype6ae.hpp"
#include "oddlib/stream.hpp"
#include "logger.hpp"
#include <vector>
#include <cassert>

namespace Oddlib
{
    static u8 NextNibble(IStream& stream, bool& readLo, u8& srcByte)
    {
        if (readLo)
        {
            readLo = !readLo;
            return srcByte >> 4;
        }
        else
        {
            srcByte = ReadU8(stream);
            readLo = !readLo;
            return srcByte & 0xF;
        }
    }

    // Function 0x0040A8A0 in AE
    std::vector<u8> CompressionType6Ae::Decompress(IStream& stream, u32 finalW, u32 w, u32 h, u32 /*dataSize*/)
    {
        std::vector<u8> out(finalW*h);

        bool bNibbleToRead = false;
        bool bSkip = false;
        u32 dstPos = 0;

        if (h > 0)
        {
            u8 srcByte = 0;
            s32 heightCounter = h;
            do
            {
                u32 widthCounter = 0;
                while (widthCounter < w)
                {
                    u8 nibble = NextNibble(stream, bNibbleToRead, srcByte);
                    widthCounter += nibble;

                    if (nibble > 0)
                    {
                        do
                        {
                            if (bSkip)
                            {
                                dstPos++;
                            }
                            else
                            {
                                out[dstPos] = 0;
                            }
                            bSkip = !bSkip;
                            --nibble;
                        } while (nibble);
                    }

                    nibble = NextNibble(stream, bNibbleToRead, srcByte);
                    widthCounter += nibble;

                    if (nibble > 0)
                    {
                        do
                        {
                            const u8 data = NextNibble(stream, bNibbleToRead, srcByte);
                            if (bSkip)
                            {
                                out[dstPos++] |= 16 * data;
                                bSkip = 0;
                            }
                            else
                            {
                                out[dstPos] = data;
                                bSkip = 1;
                            }
                        } while (--nibble);
                    }
                }

                for (; widthCounter & 7; ++widthCounter)
                {
                    if (bSkip)
                    {
                        dstPos++;
                    }
                    else
                    {
                        out[dstPos] = 0;
                    }
                    bSkip = !bSkip;
                }

            } while (heightCounter-- != 1);
        }
        return out;
    }
}
