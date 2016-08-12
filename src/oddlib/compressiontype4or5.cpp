#include "oddlib/compressiontype4or5.hpp"
#include "oddlib/stream.hpp"

namespace Oddlib
{
    // 0xxx xxxx = string of literals (1 to 128)
    // 1xxx xxyy yyyy yyyy = copy from y bytes back, x bytes
    // Function 0x004ABAB0 in AE
    std::vector<u8> CompressionType4Or5::Decompress(IStream& stream, u32 /*finalW*/, u32 /*w*/, u32 /*h*/, u32 /*dataSize*/)
    {
        stream.Seek(stream.Pos() - 4);

        // Get the length of the destination buffer
        u32 nDestinationLength = 0;
        stream.ReadUInt32(nDestinationLength);

        std::vector<u8> decompressedData(nDestinationLength);
        u32 dstPos = 0;
        while (dstPos < nDestinationLength)
        {
            // get code byte
            const u8 c = ReadUInt8(stream);

            // 0x80 = 0b10000000 = RLE flag
            // 0xc7 = 0b01111100 = bytes to use for length
            // 0x03 = 0b00000011
            if (c & 0x80)
            {
                // Figure out how many bytes to copy.
                const u32 nCopyLength = ((c & 0x7C) >> 2) + 3;

                // The last 2 bits plus the next byte gives us the destination of the copy
                const u8 c1 = ReadUInt8(stream);
                const u32 nPosition = ((c & 0x03) << 8) + c1 + 1;
                const u32 startIndex = dstPos - nPosition;

                for (u32 i = 0; i < nCopyLength; i++)
                {
                    decompressedData[dstPos++] = decompressedData[startIndex+i];
                }
            }
            else
            {
                // Here the value is the number of literals to copy
                for (int i = 0; i < c + 1; i++)
                {
                    decompressedData[dstPos++] = ReadUInt8(stream);
                }
            }
        }
        return decompressedData;
    }
}
