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


        union intUni
        {
            Uint8 b[4];
            int i;
        } dstByte;


        bool bNibbleToRead = false; // ebp@5
        bool bBitsWriten = false;
        
        Uint32 dstPos = 0;

        if (h > 0)
        {
            Uint8 srcByte = 0;
            Sint32 heightCounter = h;
            do
            {
                Uint32 byteCounter = 0;
                while (byteCounter < w)
                {
                    Uint8 nibble = NextNibble(stream, bNibbleToRead, srcByte);
                    byteCounter += nibble;

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
                            --nibble;
                        } while (nibble);
                    }

                    nibble = NextNibble(stream, bNibbleToRead, srcByte);
                    byteCounter += nibble;

                    if (nibble > 0)
                    {
                        do
                        {
                            dstByte.b[0] = NextNibble(stream, bNibbleToRead, srcByte);
                         

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
                        } while (--nibble);
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
