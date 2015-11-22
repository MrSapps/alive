#include "oddlib/compressiontype6or7aepsx.hpp"
#include "oddlib/stream.hpp"
#include "logger.hpp"
#include <vector>
#include <cassert>

namespace Oddlib
{
    template<Uint32 BitsSize>
    Uint32 NextBits(IStream& stream, unsigned int& bitCounter, unsigned int& srcWorkBits, const signed int kFixedMask)
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
        std::vector<Uint8> out(finalW*h * 2);

        // TODO: std::array
        unsigned char tmp1[256] = {};
        unsigned char tmp2[256] = {};
        unsigned char tmp3[256] = {};

        const signed int kFixedMask = 1 << BitsSize;
        const unsigned int v36 = ((unsigned int)(kFixedMask) >> 1) - 1;

        unsigned int bitCounter = 0;
        unsigned int srcWorkBits = 0;

        const auto kStartPos = stream.Pos();
        const unsigned int kInputSize = (BitsSize * dataSize) >> 3;
        while (stream.Pos() < kStartPos+kInputSize)
        {
            unsigned int count = 0;
            do
            {
                const unsigned int maskedSrcBits1 = NextBits<BitsSize>(stream, bitCounter, srcWorkBits, kFixedMask);

                int maskedSrcBits1Copy = maskedSrcBits1;

                if (maskedSrcBits1 > v36)
                {
                    int remainder = maskedSrcBits1 - v36;
                    maskedSrcBits1Copy = remainder;
                    if (remainder)
                    {
                        do
                        {
                            tmp2[count] = static_cast<char>(count);
                            ++count;
                            --remainder;
                        } while (remainder);
                        maskedSrcBits1Copy = remainder;
                    }
                }

                if (count == kFixedMask)
                {
                    break;
                }

                unsigned int count2 = maskedSrcBits1Copy + 1;

                for (;;)
                {
                    const unsigned int v14 = NextBits<BitsSize>(stream, bitCounter, srcWorkBits, kFixedMask);
                    
                    // TODO: bounds safe
                    *(&tmp1[count] + (tmp2 - tmp1)) = static_cast<char>(v14);

                    if (count != v14)
                    {
                        const unsigned int v16 = NextBits<BitsSize>(stream, bitCounter, srcWorkBits, kFixedMask);
                        tmp1[count] = static_cast<unsigned char>(v16); 
                    }

                    ++count;

                    if (count2-- == 1)
                    {
                        break;
                    }
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
