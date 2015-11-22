#include "oddlib/compressiontype6or7aepsx.hpp"
#include "oddlib/stream.hpp"
#include "logger.hpp"
#include <vector>
#include <cassert>

namespace Oddlib
{
    // Function 0x004ABB90 in AE, function 0x8005B09C in AE PSX demo
    template<Uint32 BitsSize>
    std::vector<Uint8> CompressionType6or7AePsx<BitsSize>::Decompress(IStream& stream, Uint32 finalW, Uint32 /*w*/, Uint32 h, Uint32 dataSize)
    {
        
        std::vector<Uint8> out(finalW*h * 400);
        std::vector<Uint8> in(dataSize);
        stream.ReadBytes(in.data(), in.size());

        Uint16* pInput = (Uint16*)in.data();
        Uint8* pOutput = (Uint8*)out.data();
        int whDWORD = dataSize;
       // const int b8or6 = 6;// 8;// w == 8 ? 8 : 6;

        unsigned int bitCounter = 0; // edx@1
        Uint16 *pSrc1 = 0; // ebp@1
        unsigned int srcWorkBits = 0; // esi@1
        int count = 0; // eax@2
        int srcBits = 0; // ebx@4
        unsigned int maskedSrcBits1 = 0; // ebx@5
        int remainder = 0; // ebx@6
        int remainderCopy = 0; // ecx@7
        int v12 = 0; // ebx@14
        int v13 = 0; // ebp@15
        int v14 = 0; // ebx@15
        int v15 = 0; // ebx@17
        char v16 = 0; // bl@18
        char bLastByte = 0; // zf@19
        char v18 = 0; // cl@22
        int v19 = 0; // eax@23
        unsigned int v20 = 0; // edx@23
        unsigned int v21 = 0; // esi@23
        char v22 = 0; // cl@24
        int v23 = 0; // eax@25
        int v24 = 0; // ebx@27
        int v25 = 0; // ebx@28
        int v26 = 0; // ebx@30
        int i = 0; // ebp@32
        unsigned char v28 = 0; // cl@33
        Uint16 *pSrcCopy = 0; // [sp+Ch] [bp-31Ch]@1
        int maskedSrcBits1Copy = 0; // [sp+10h] [bp-318h]@5
        int count2 = 0; // [sp+10h] [bp-318h]@11
        int v33 = 0; // [sp+10h] [bp-318h]@25
        signed int kFixedMask = 0; // [sp+14h] [bp-314h]@1
       // Uint8 *pOutputStart = 0; // [sp+1Ch] [bp-30Ch]@1
        unsigned int v36 = 0; // [sp+24h] [bp-304h]@1
        char tmp1[256] = {}; // [sp+28h] [bp-300h]@15
        char tmp2[256] = {}; // [sp+128h] [bp-200h]@8
        unsigned char tmp3[256] = {}; // [sp+228h] [bp-100h]@27

//        pOutputStart = pOutput;
        srcWorkBits = 0;
        bitCounter = 0;
        pSrc1 = pInput;
        pSrcCopy = pInput;
        kFixedMask = 1 << BitsSize;                      // could have been the bit depth?
        v36 = ((unsigned int)(1 << BitsSize) >> 1) - 1;
        while (pSrc1 < (Uint16 *)((char *)pInput + ((unsigned int)(BitsSize * whDWORD) >> 3)))// could be the first dword of the frame which is usually the size?
        {
            count = 0;
            do
            {
                if (bitCounter < 0x10)
                {
                    srcBits = *pSrc1 << bitCounter;
                    bitCounter += 16;
                    srcWorkBits |= srcBits;
                    ++pSrc1;
                    pSrcCopy = pSrc1;
                }
                bitCounter -= BitsSize;
                maskedSrcBits1 = srcWorkBits & (kFixedMask - 1);
                srcWorkBits >>= BitsSize;
                maskedSrcBits1Copy = maskedSrcBits1;
                if (maskedSrcBits1 > v36)
                {
                    remainder = maskedSrcBits1 - v36;
                    maskedSrcBits1Copy = remainder;
                    if (remainder)
                    {
                        remainderCopy = remainder;
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
                    break;
                count2 = maskedSrcBits1Copy + 1;
                while (1)
                {
                    if (bitCounter < 0x10)
                    {
                        v12 = *pSrc1 << bitCounter;
                        bitCounter += 16;
                        srcWorkBits |= v12;
                        pSrcCopy = pSrc1 + 1;
                    }
                    bitCounter -= BitsSize;
                    v13 = kFixedMask - 1;
                    v14 = srcWorkBits & (kFixedMask - 1);
                    srcWorkBits >>= BitsSize;
                    *(&tmp1[count] + (tmp2 - tmp1)) = static_cast<char>(v14);
                    if (count != v14)
                    {
                        if (bitCounter < 0x10)
                        {
                            v15 = *pSrcCopy << bitCounter;
                            bitCounter += 16;
                            srcWorkBits |= v15;
                            ++pSrcCopy;
                        }
                        v16 = static_cast<char>(srcWorkBits & v13);
                        bitCounter -= BitsSize;
                        srcWorkBits >>= BitsSize;
                        tmp1[count] = v16;
                    }
                    ++count;
                    bLastByte = count2-- == 1;
                    if (bLastByte)
                        break;
                    pSrc1 = pSrcCopy;
                }
                pSrc1 = pSrcCopy;
            } while (count != kFixedMask);
            if (bitCounter < 0x10)
            {
                v18 = static_cast<char>(bitCounter);
                bitCounter += 16;
                srcWorkBits |= *pSrc1 << v18;
                ++pSrc1;
                pSrcCopy = pSrc1;
            }
            v20 = bitCounter - BitsSize;
            v19 = (srcWorkBits & (kFixedMask - 1)) << BitsSize;
            v21 = srcWorkBits >> BitsSize;
            if (v20 < 0x10)
            {
                v22 = static_cast<char>(v20);
                v20 += 16;
                v21 |= *pSrc1 << v22;
                ++pSrc1;
                pSrcCopy = pSrc1;
            }
            bitCounter = v20 - BitsSize;
            v33 = (v21 & (kFixedMask - 1)) + v19;
            srcWorkBits = v21 >> BitsSize;
            v23 = 0;
            while (1)
            {
                if (v23)
                {
                    --v23;
                    v24 = tmp3[v23];
                    goto LABEL_32;
                }
                v25 = v33--;
                if (!v25)
                    break;
                if (bitCounter < 0x10)
                {
                    v26 = *pSrc1 << bitCounter;
                    bitCounter += 16;
                    srcWorkBits |= v26;
                    pSrcCopy = pSrc1 + 1;
                }
                bitCounter -= BitsSize;
                v24 = srcWorkBits & (kFixedMask - 1);
                srcWorkBits >>= BitsSize;
            LABEL_32:
                for (i = (unsigned char)tmp2[v24]; v24 != i; i = (unsigned char)tmp2[i])
                {
                    v28 = tmp1[v24];
                    v24 = i;
                    tmp3[v23++] = v28;
                }
                pSrc1 = pSrcCopy;
                *pOutput++ = static_cast<Uint8>(v24);
            }
        }
        // return pOutput - pOutputStart;
        return out;
       
    }

    // Explicit template instantiation
    template class CompressionType6or7AePsx<6>;
    template class CompressionType6or7AePsx<8>;
}
