#include "oddlib/compressiontype4or5.hpp"
#include "oddlib/stream.hpp"

namespace Oddlib
{
    // 0xxx xxxx = string of literals (1 to 128)
    // 1xxx xxyy yyyy yyyy = copy from y bytes back, x bytes
    std::vector<Uint8> CompressionType4Or5::Decompress(IStream& stream, Uint32 /*finalW*/, Uint32 /*w*/, Uint32 /*h*/, Uint32 /*dataSize*/)
    {
        stream.Seek(stream.Pos() - 4);

        // Get the length of the destination buffer
        Uint32 nDestinationLength = 0;
        stream.ReadUInt32(nDestinationLength);

        std::vector<Uint8> decompressedData(nDestinationLength);
        Uint32 dstPos = 0;
        while (dstPos < nDestinationLength)
        {
            // HACK is, 0x1, 0xF where 0xf is the "real" byte we want to copy!
            unsigned char c = 0;
            stream.ReadUInt8(c); // get code byte

            // 0x80 = 0b10000000 = RLE flag
            // 0xc7 = 0b01111100 = bytes to use for length
            // 0x03 = 0b00000011
            if (c & 0x80)
            {
                // Figure out how many bytes to copy.
                const unsigned int nCopyLength = ((c & 0x7C) >> 2) + 3;

                // The last 2 bits plus the next byte gives us the destination of the copy
                unsigned char c1 = 0;
                stream.ReadUInt8(c1);
                const unsigned int nPosition = ((c & 0x03) << 8) + c1 + 1;

                const Uint32 startIndex = dstPos - nPosition;
                const Uint32 endIndex = dstPos + nCopyLength;
                for (Uint32 i = startIndex; i < endIndex; i++)
                {
                    // TODO FIX ME
                    // decompressedData[dstPos++] = decompressedData[i];
                }
                dstPos += nCopyLength;
            }
            else
            {
                // Here the value is the number of literals to copy
                for (int i = 0; i < c + 1; i++)
                {
                    Uint8 tmp = 0;
                    stream.ReadUInt8(tmp);
                    decompressedData[dstPos] = tmp;
                    dstPos++;
                }
            }
        }

        return decompressedData;
    }
}
