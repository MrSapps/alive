#include "oddlib/compressiontype6or7aepsx.hpp"
#include "oddlib/stream.hpp"
#include "logger.hpp"
#include <vector>
#include <cassert>

namespace Oddlib
{
    template<Uint32 BitsSize, typename OutType>
    void NextBits(unsigned int& bitCounter, unsigned int& srcWorkBits, Uint16 *&pSrc1, Uint16 *&pSrcCopy, const signed int kFixedMask, OutType& maskedSrcBits1)
    {
        if (bitCounter < 16)
        {
            const int srcBits = *pSrc1 << bitCounter;
            bitCounter += 16;
            srcWorkBits |= srcBits;
            ++pSrc1;
            pSrcCopy = pSrc1;
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

        std::vector<Uint8> out(finalW*h * 4);
        std::vector<Uint8> in(kInputSize);
        stream.ReadBytes(in.data(), in.size());

        Uint16* pInput = (Uint16*)in.data();
        Uint8* pOutput = (Uint8*)out.data();
  
        unsigned int bitCounter = 0; // edx@1
        Uint16 *pSrc1 = pInput; // ebp@1
        Uint16 *pSrcCopy = pInput; // [sp+Ch] [bp-31Ch]@1

        unsigned int srcWorkBits = 0; // esi@1
        int count = 0; // eax@2
        unsigned int maskedSrcBits1 = 0; // ebx@5
        char bLastByte = 0; // zf@19
        char v16 = 0; // bl@18

        unsigned char v28 = 0; // cl@33

        int count2 = 0; // [sp+10h] [bp-318h]@11
        int v33 = 0; // [sp+10h] [bp-318h]@25

        unsigned char tmp1[256] = {}; // [sp+28h] [bp-300h]@15
        unsigned char tmp2[256] = {}; // [sp+128h] [bp-200h]@8
        unsigned char tmp3[256] = {}; // [sp+228h] [bp-100h]@27

        const signed int kFixedMask = 1 << BitsSize;
        
        const unsigned int v36 = ((unsigned int)(kFixedMask) >> 1) - 1;

        while (pSrc1 < (Uint16 *)( ((char *)pInput) + kInputSize))// could be the first dword of the frame which is usually the size?
        {
            count = 0;
            do
            {
                NextBits<BitsSize>(bitCounter, srcWorkBits, pSrc1, pSrcCopy, kFixedMask, maskedSrcBits1);

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

                count2 = maskedSrcBits1Copy + 1;
                for (;;)
                {
                    int v14 = 0;
                    NextBits<BitsSize>(bitCounter, srcWorkBits, pSrc1, pSrcCopy, kFixedMask, v14);
                    *(&tmp1[count] + (tmp2 - tmp1)) = static_cast<char>(v14);
                    if (count != v14)
                    {
                        NextBits<BitsSize>(bitCounter, srcWorkBits, pSrc1, pSrcCopy, kFixedMask, v16);
                        tmp1[count] = static_cast<unsigned char>(v16); 
                    }
                    ++count;
                    bLastByte = count2-- == 1;
                    if (bLastByte)
                    {
                        break;
                    }

                    pSrc1 = pSrcCopy; // dead?
                }
                pSrc1 = pSrcCopy; // dead?
            } while (count != kFixedMask);

            int v19 = 0;
            NextBits<BitsSize>(bitCounter, srcWorkBits, pSrc1, pSrcCopy, kFixedMask, v19);
            v19 = v19 << BitsSize; // Extra

            NextBits<BitsSize>(bitCounter, srcWorkBits, pSrc1, pSrcCopy, kFixedMask, v33);
            v33 = v33 + v19; // Extra

            int v23 = 0;
            int v24 = 0;
            for (;;)
            {
                if (v23)
                {
                    --v23;
                    v24 = tmp3[v23];
                }
                else
                {
                    const int v25 = v33--;
                    if (!v25)
                    {
                        break;
                    }

                    NextBits<BitsSize>(bitCounter, srcWorkBits, pSrc1, pSrcCopy, kFixedMask, v24);
                }

                for (int i = tmp2[v24]; v24 != i; i = tmp2[i])
                {
                    v28 = tmp1[v24];
                    v24 = i;
                    tmp3[v23++] = v28;
                }
                pSrc1 = pSrcCopy; // dead?
                *pOutput++ = static_cast<Uint8>(v24);
            }
        }

        return out;
    }

    // Explicit template instantiation
    template class CompressionType6or7AePsx<6>;
    template class CompressionType6or7AePsx<8>;
}
