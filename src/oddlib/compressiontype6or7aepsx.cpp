#include "oddlib/compressiontype6or7aepsx.hpp"
#include "oddlib/stream.hpp"
#include "logger.hpp"
#include <vector>
#include <array>
#include <cassert>

namespace Oddlib
{
    template<Uint32 BitsSize>
    static Uint32 NextBits(IStream& stream, unsigned int& bitCounter, unsigned int& srcWorkBits, const signed int kFixedMask)
    {
        if (bitCounter < 16)
        {
            const int srcBits = ReadUint16(stream) << bitCounter;
            bitCounter += 16;
            srcWorkBits |= srcBits;
        }

        bitCounter -= BitsSize;
        const Uint32 ret = srcWorkBits & (kFixedMask - 1);
        srcWorkBits >>= BitsSize;
        return ret;
    }

    // Function 0x004ABB90 in AE, function 0x8005B09C in AE PSX demo
    template<Uint32 BitsSize>
    std::vector<Uint8> CompressionType6or7AePsx<BitsSize>::Decompress(IStream& stream, Uint32 finalW, Uint32 /*w*/, Uint32 h, Uint32 dataSize)
    {
        Uint32 outputPos = 0;
        std::vector<Uint8> out(finalW*h*2);

        std::array<unsigned char,256> tmp1 = {};
        std::array<unsigned char, 256> tmp2 = {};
        std::array<unsigned char, 256> tmp3 = {};

        const unsigned int kFixedMask = 1 << BitsSize;
        const unsigned int kInvertedFixedMask = ((kFixedMask) >> 1) - 1;

        unsigned int bitCounter = 0;
        unsigned int srcWorkBits = 0;

        const auto kStartPos = stream.Pos();
        const unsigned int kInputSize = (BitsSize * dataSize) >> 3;
        while (stream.Pos() < kStartPos+kInputSize)
        {
            unsigned int count = 0;
            do
            {
                unsigned int maskedSrcBits1 = NextBits<BitsSize>(stream, bitCounter, srcWorkBits, kFixedMask);

                if (maskedSrcBits1 > kInvertedFixedMask)
                {
                    int remainder = maskedSrcBits1 - kInvertedFixedMask;
                    while (remainder != 0)
                    {
                        tmp2[count] = static_cast<char>(count);
                        ++count;
                        --remainder;
                    }
                    maskedSrcBits1 = remainder;
                }

                if (count == kFixedMask)
                {
                    break;
                }

                unsigned int count2 = maskedSrcBits1 + 1;

                while (count2 != 0)
                {
                    unsigned int bits = NextBits<BitsSize>(stream, bitCounter, srcWorkBits, kFixedMask);
                    tmp2[count] = static_cast<char>(bits);
                    if (count != bits)
                    {
                        tmp1[count] = static_cast<unsigned char>(NextBits<BitsSize>(stream, bitCounter, srcWorkBits, kFixedMask));
                    }

                    ++count;
                    --count2;
                }
            } while (count != kFixedMask);

            const unsigned int counterPart = NextBits<BitsSize>(stream, bitCounter, srcWorkBits, kFixedMask) << BitsSize;
            unsigned int counter = NextBits<BitsSize>(stream, bitCounter, srcWorkBits, kFixedMask) + counterPart;

            int tmp2Idx = 0;
            for (;;)
            {
                unsigned int tmp1Idx = 0;
                if (tmp2Idx)
                {
                    --tmp2Idx;
                    tmp1Idx = tmp3[tmp2Idx];
                }
                else
                {
                    if (!counter--)
                    {
                        break;
                    }

                    tmp1Idx = NextBits<BitsSize>(stream, bitCounter, srcWorkBits, kFixedMask);
                }

                for (unsigned int i = tmp2[tmp1Idx]; tmp1Idx != i; i = tmp2[i])
                {
                    tmp3[tmp2Idx++] = tmp1[tmp1Idx];
                    tmp1Idx = i;
                }

                out[outputPos++] = static_cast<Uint8>(tmp1Idx);
            }
        }

        return out;
    }

    // Explicit template instantiation
    template class CompressionType6or7AePsx<6>;
    template class CompressionType6or7AePsx<8>;
}
