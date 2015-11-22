#include "oddlib/compressiontype6or7aepsx.hpp"
#include "oddlib/stream.hpp"
#include "logger.hpp"
#include <vector>
#include <cassert>

namespace Oddlib
{
    template<Uint32 BitsSize, typename OutType>
    void NextBits(IStream& stream, unsigned int& bitCounter, unsigned int& srcWorkBits, const signed int kFixedMask, OutType& maskedSrcBits1)
    {
        if (bitCounter < 16)
        {
            const int srcBits = ReadUint16(stream) << bitCounter;
            bitCounter += 16;
            srcWorkBits |= srcBits;
        }

        bitCounter -= BitsSize;
        maskedSrcBits1 = static_cast<OutType>(srcWorkBits & (kFixedMask - 1));
        srcWorkBits >>= BitsSize;
    }

    // Function 0x004ABB90 in AE, function 0x8005B09C in AE PSX demo
    template<Uint32 BitsSize>
    std::vector<Uint8> CompressionType6or7AePsx<BitsSize>::Decompress(IStream& stream, Uint32 finalW, Uint32 /*w*/, Uint32 h, Uint32 dataSize)
    {
        unsigned int kInputSize = (BitsSize * dataSize) >> 3;

        std::vector<Uint8> out(finalW*h * 2);

        Uint32 outputPos = 0;

        unsigned char tmp1[256] = {};
        unsigned char tmp2[256] = {};
        unsigned char tmp3[256] = {};

        const signed int kFixedMask = 1 << BitsSize;
        const unsigned int v36 = ((unsigned int)(kFixedMask) >> 1) - 1;

        unsigned int bitCounter = 0;

        unsigned int srcWorkBits = 0; // Must live for as long as the outer most loop/scope
        const auto kStartPos = stream.Pos();
        while (stream.Pos() < kStartPos+kInputSize)
        {
            int count = 0;
            do
            {
                int maskedSrcBits1 = 0;
                NextBits<BitsSize>(stream, bitCounter, srcWorkBits, kFixedMask, maskedSrcBits1);

                int maskedSrcBits1Copy = maskedSrcBits1;

                if (maskedSrcBits1 > v36)
                {
                    int remainder = maskedSrcBits1 - v36;
                    maskedSrcBits1Copy = remainder;
                    if (remainder)
                    {
                        int remainderCopy = remainder;
                        do
                        {
                            tmp2[count] = static_cast<char>(count);
                            ++count;
                            --remainder;
                            --remainderCopy;
                        } while (remainderCopy);
                        maskedSrcBits1Copy = remainder;
                    }
                }

                if (count == kFixedMask)
                {
                    break;
                }

                int count2 = maskedSrcBits1Copy + 1;
                for (;;)
                {
                    int v14 = 0;
                    NextBits<BitsSize>(stream, bitCounter, srcWorkBits, kFixedMask, v14);
                    *(&tmp1[count] + (tmp2 - tmp1)) = static_cast<char>(v14);
                    if (count != v14)
                    {
                        char v16; // TODO: int?
                        NextBits<BitsSize>(stream, bitCounter, srcWorkBits, kFixedMask, v16);
                        tmp1[count] = static_cast<unsigned char>(v16); 
                    }
                    ++count;
                    const char bLastByte = count2-- == 1;
                    if (bLastByte)
                    {
                        break;
                    }
                }
            } while (count != kFixedMask);

            int v19 = 0;
            NextBits<BitsSize>(stream, bitCounter, srcWorkBits, kFixedMask, v19);
            v19 = v19 << BitsSize; // Extra

            int v33 = 0;
            NextBits<BitsSize>(stream, bitCounter, srcWorkBits, kFixedMask, v33);
            v33 = v33 + v19; // Extra

            int v23 = 0;
            for (;;)
            {
                int v24 = 0;
                if (v23)
                {
                    --v23;
                    v24 = tmp3[v23];
                }
                else
                {
                    if (!v33--)
                    {
                        break;
                    }

                    NextBits<BitsSize>(stream, bitCounter, srcWorkBits, kFixedMask, v24);
                }

                for (int i = tmp2[v24]; v24 != i; i = tmp2[i])
                {
                    unsigned char v28 = tmp1[v24];
                    v24 = i;
                    tmp3[v23++] = v28;
                }
                out[outputPos++] = static_cast<Uint8>(v24);
            }
        }

        return out;
    }

    // Explicit template instantiation
    template class CompressionType6or7AePsx<6>;
    template class CompressionType6or7AePsx<8>;
}
