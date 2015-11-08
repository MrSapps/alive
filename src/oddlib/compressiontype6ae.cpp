#include "oddlib/compressiontype6ae.hpp"
#include "oddlib/stream.hpp"
#include "logger.hpp"
#include <vector>
#include <cassert>

namespace Oddlib
{
    static Uint8 NextNibble(IStream& stream, bool& readLo, Uint8& srcByte)
    {
        if (readLo)
        {
            readLo = !readLo;
            return srcByte >> 4;
        }
        else
        {
            srcByte = ReadUInt8(stream);
            readLo = !readLo;
            return srcByte & 0xF;
        }
    }

    // Function 0x0040A8A0 in AE
    std::vector<Uint8> CompressionType6Ae::Decompress(IStream& stream, Uint32 finalW, Uint32 w, Uint32 h, Uint32 /*dataSize*/)
    {
        std::vector<Uint8> out(finalW*h);

        bool bNibbleToRead = false;
        bool bSkip = false;
        Uint32 dstPos = 0;

        if (h > 0)
        {
            Uint8 srcByte = 0;
            Sint32 heightCounter = h;
            do
            {
                Uint32 widthCounter = 0;
                while (widthCounter < w)
                {
                    Uint8 nibble = NextNibble(stream, bNibbleToRead, srcByte);
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
                            const Uint8 data = NextNibble(stream, bNibbleToRead, srcByte);
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
