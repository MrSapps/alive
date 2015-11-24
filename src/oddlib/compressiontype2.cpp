#include "oddlib/compressiontype2.hpp"
#include "oddlib/stream.hpp"
#include "logger.hpp"

namespace Oddlib
{
    static bool Expand3To4Bytes(Sint32& remainingCount, IStream& stream, std::vector<Uint8>& ret, Uint32& dstPos)
    {
        if (!remainingCount)
        {
            return false;
        }
        const Sint32 src3Bytes = ReadUInt8(stream) | (ReadUint16(stream) << 8);
        remainingCount--;

        // TODO: Should write each byte by itself
        const Uint32 value = (4 * (Uint16)src3Bytes & 0x3F00) | (src3Bytes & 0x3F) | (16 * src3Bytes & 0x3F0000) | (4 * (16 * src3Bytes & 0xFC00000));
        *reinterpret_cast<Uint32*>(&ret[dstPos]) = value;
        dstPos += 4;

        return true;
    }

    // Function 0x0040AA50 in AE
    std::vector<Uint8> CompressionType2::Decompress(IStream& stream, Uint32 finalW, Uint32 /*w*/, Uint32 h, Uint32 dataSize)
    {
        // HACK: Add on 43 DWORD buffer overrun area - some AE PSX sprites write
        // this far out of bounds - just cropping off the extra
        // pixels is a good enough workaround.
        std::vector<Uint8> ret((finalW*h)+(4*43));

        Sint32 dwords_left = dataSize / 4;
        Sint32 remainder = dataSize % 4;

        Uint32 dstPos = 0;
        if (dwords_left > 0)
        {
            do
            {
                for (int i = 0; i < 4; i++)
                {
                    if (!Expand3To4Bytes(dwords_left, stream, ret, dstPos))
                    {
                        break;
                    }
                }
            } while (dwords_left);
        }

        // TODO: Branch not tested - copies remainder bytes directly into output
        while (remainder)
        {
            remainder--;
            ret[dstPos++] = ReadUInt8(stream);
        }

        return ret;
    }
}
