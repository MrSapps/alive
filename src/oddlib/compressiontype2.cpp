#include "oddlib/compressiontype2.hpp"
#include "oddlib/stream.hpp"
#include "logger.hpp"

namespace Oddlib
{
    static bool ExpandElement(Sint32& remainingCount, Uint8*& src, Uint32*& dst)
    {
        if (!remainingCount)
        {
            return false;
        }
        const Sint32 src_word = *src | (*(Uint16 *)(src + 1) << 8);
        ++dst;
        src += 3;
        remainingCount--;
        *(dst - 1) = 4 * (Uint16)src_word & 0x3F00 | src_word & 0x3F | 16 * src_word & 0x3F0000 | 4 * (16 * src_word & 0xFC00000);

        return true;
    }

    // Function 0x0040AA50 in AE
    std::vector<Uint8> CompressionType2::Decompress(IStream& stream, Uint32 finalW, Uint32 /*w*/, Uint32 h, Uint32 dataSize)
    {
        std::vector<Uint8> ret(finalW*h);

        std::vector<Uint8> s(dataSize);
        stream.ReadBytes(s.data(), s.size());

        Sint32 dwords_left = dataSize / 4;
        Sint32 remainder = dataSize % 4;

        Uint32 *dst = (Uint32*)ret.data();
        Uint8 *src = s.data();

        if (dwords_left > 0)
        {
            dst = (Uint32*)ret.data();
            src = s.data();
            do
            {
                for (int i = 0; i < 4; i++)
                {
                    if (!ExpandElement(dwords_left, src, dst))
                    {
                        break;
                    }
                }
            } while (dwords_left);
        }

        while (remainder)
        {
            remainder--;
            *(Uint8 *)dst = *src;
            dst = (Uint32 *)((char *)dst + 1);
            ++src;
        }

        return ret;
    }
}
