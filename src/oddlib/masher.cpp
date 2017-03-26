#include "oddlib/masher.hpp"
#include "oddlib/lvlarchive.hpp"
#include "oddlib/masher_tables.hpp"
#include "logger.hpp"
#include <assert.h>
#include <array>

namespace Oddlib
{
    void Masher::Read()
    {
        mStream->Read(mFileHeader.mDdvTag);
        if (mFileHeader.mDdvTag != MakeType("DDV\0"))
        {
            LOG_ERROR("Invalid DDV magic tag " << mFileHeader.mDdvTag);
            throw InvalidDdv("Invalid DDV tag");
        }

        mStream->Read(mFileHeader.mDdvVersion);
        if (mFileHeader.mDdvVersion != 1)
        {
            // This is the only version seen in all of the known data
            LOG_ERROR("Expected DDV version to be 2 but got " << mFileHeader.mDdvVersion);
            throw InvalidDdv("Wrong DDV version");
        }

        mStream->Read(mFileHeader.mContains);
        mStream->Read(mFileHeader.mFrameRate);
        mStream->Read(mFileHeader.mNumberOfFrames);

        mbHasVideo = (mFileHeader.mContains & 0x1) == 0x1;
        mbHasAudio = (mFileHeader.mContains & 0x2) == 0x2;

        if (mbHasVideo)
        {
            mStream->Read(mVideoHeader.mUnknown);
            mStream->Read(mVideoHeader.mWidth);
            mStream->Read(mVideoHeader.mHeight);
            mStream->Read(mVideoHeader.mMaxAudioFrameSize);
            mStream->Read(mVideoHeader.mMaxVideoFrameSize);
            mStream->Read(mVideoHeader.mKeyFrameRate);

            mNumMacroblocksX = (mVideoHeader.mWidth / 16);
            if (mVideoHeader.mWidth % 16 != 0)
            {
                mNumMacroblocksX++;
            }

            mNumMacroblocksY = (mVideoHeader.mHeight / 16);
            if (mVideoHeader.mHeight % 16 != 0)
            {
                mNumMacroblocksY++;
            }
        }

        if (mbHasAudio)
        {
            mStream->Read(mAudioHeader.mAudioFormat);
            mStream->Read(mAudioHeader.mSampleRate);
            mStream->Read(mAudioHeader.mMaxAudioFrameSize);
            mStream->Read(mAudioHeader.mSingleAudioFrameSize);
            mStream->Read(mAudioHeader.mNumberOfFramesInterleave);


            for (uint32_t i = 0; i < mAudioHeader.mNumberOfFramesInterleave; i++)
            {
                uint32_t tmp = 0;
                mStream->Read(tmp);
                mAudioFrameSizes.emplace_back(tmp);
            }
        }

        for (uint32_t i = 0; i < mFileHeader.mNumberOfFrames; i++)
        {
            uint32_t tmp = 0;
            mStream->Read(tmp);
            mFrameSizes.emplace_back(tmp);
        }

        // TODO: Read/skip mAudioHeader.mNumberOfFramesInterleave frame datas
        for (auto i = 0u; i < mAudioHeader.mNumberOfFramesInterleave; i++)
        {
            const uint32_t totalSize = mAudioFrameSizes[i];
            mStream->Seek(mStream->Pos() + totalSize);
        }

        mMacroBlockBuffer.resize(mNumMacroblocksX * mNumMacroblocksY * 16 * 16 * 6);

        mDecodedVideoFrameData.resize(mVideoHeader.mMaxVideoFrameSize);

        // TODO: Need work buffer for audio
        mAudioFrameData.resize(mVideoHeader.mMaxAudioFrameSize);
    }

    static u16 GetHiWord(u32 v)
    {
        return static_cast<u16>((v >> 16) & 0xFFFF);
    }

#ifndef MAKELONG
#define MAKELONG(a, b)      ((((u16)(((a)) & 0xffff)) | ((u32)((u16)(((b)) & 0xffff))) << 16))
#endif

    static void SetLoWord(u32& v, u16 lo)
    {
        u16 hiWord = GetHiWord(v);
        v = MAKELONG(lo, hiWord);
    }

    static void SetHiWord(u32& v, u16 hi)
    {
        u16 loWord = v & 0xFFFF;
        v = MAKELONG(loWord, hi);
    }


    static inline void CheckForEscapeCode(char& bitsToShiftBy, int& rawWord1, u16*& rawBitStreamPtr, u32& rawWord4, u32& v25)
    {
        // I think this is used as an escape code?
        if (bitsToShiftBy & 16)   // 0b10000 if bit 5 set
        {
            rawWord1 = *rawBitStreamPtr;
            ++rawBitStreamPtr;

            bitsToShiftBy &= 15;
            rawWord4 = rawWord1 << bitsToShiftBy;
            v25 |= rawWord4;
        }
    }

    static inline void OutputWordAndAdvance(u16*& rawBitStreamPtr, u32& rawWord4, unsigned short int*& pOut, char& numBitsToShiftBy, u32& v3)
    {
        *pOut++ = v3 >> (32 - 16);

        rawWord4 = *rawBitStreamPtr++ << numBitsToShiftBy;
        v3 = rawWord4 | (v3 << 16);
    }

#define MASK_11_BITS 0x7FF
#define MASK_10_BITS 0x3FF
#define MASK_13_BITS 0x1FFF
#define MDEC_END 0xFE00u

    static u32 GetBits(u32 value, u32 numBits)
    {
        return value >> (32 - numBits);
    }

    static void SkipBits(u32& value, char numBits, char& bitPosCounter)
    {
        value = value << numBits;
        bitPosCounter += numBits;
    }

    int decode_bitstream(u16 *pFrameData, unsigned short int *pOutput)
    {

        unsigned int table_index_2 = 0;
        int ret = *pFrameData;

        u32 v8 = ((pFrameData[2]<<16) | (pFrameData[1]));

        u16* rawBitStreamPtr = (pFrameData + 3);

        v8 = (v8 << 16) | (v8 >> 16); // Swap words

        u32 rawWord4 = GetBits(v8, 11);

        char bitsShiftedCounter = 0;
        SkipBits(v8, 11, bitsShiftedCounter);
        u32 v3 = v8;

        *pOutput++ = static_cast<unsigned short>(rawWord4); // store in output

        while (1)
        {
            do
            {
                while (1)
                {
                    do
                    {
                        while (1)
                        {
                            do
                            {
                                while (1)
                                {
                                    while (1)
                                    {
                                        table_index_2 = GetBits(v3, 13); // 0x1FFF / 8191 table size? 8192/8=1024 entries?
                                        if (table_index_2 >= 32)
                                        {
                                            break;
                                        }
                                        const int table_index_1 = GetBits(v3, 17); // 0x1FFFF / 131072, 131072/4=32768 entries?

                                        SkipBits(v3, 8, bitsShiftedCounter);

                                        int rawWord1;
                                        CheckForEscapeCode(bitsShiftedCounter, rawWord1, rawBitStreamPtr, rawWord4, v3);


                                        const char bitsToShiftFromTbl = gTbl1[table_index_1].mBitsToShift;

                                        SkipBits(v3, bitsToShiftFromTbl, bitsShiftedCounter);

                                        int rawWord2;
                                        CheckForEscapeCode(bitsShiftedCounter, rawWord2, rawBitStreamPtr, rawWord4, v3);

                                        // Everything in the table is 0's after 4266 bytes 4266/2=2133 to perhaps 2048/4096 is max?
                                        *pOutput++ = gTbl1[table_index_1].mOutputWord;

                                    } // End while


                                    const char tblValueBits = gTbl2[table_index_2].mBitsToShift;

                                    SkipBits(v3, tblValueBits, bitsShiftedCounter);

                                    int rawWord3;
                                    CheckForEscapeCode(bitsShiftedCounter, rawWord3, rawBitStreamPtr, rawWord4, v3);

                                    SetLoWord(rawWord4, gTbl2[table_index_2].mOutputWord1);

                                    if ((u16)rawWord4 != 0x7C1F) // 0b 11111 00000 11111
                                    {
                                        break;
                                    }

                                    OutputWordAndAdvance(rawBitStreamPtr, rawWord4, pOutput, bitsShiftedCounter, v3);
                                } // End while

                                *pOutput++ = static_cast<unsigned short>(rawWord4);

                                if ((u16)rawWord4 == MDEC_END)
                                {
                                    const int v15 = GetBits(v3, 11);
                                    SkipBits(v3, 11, bitsShiftedCounter);

                                    if (v15 == MASK_10_BITS)
                                    {
                                        return ret;
                                    }

                                    rawWord4 = v15 & MASK_11_BITS;
                                    *pOutput++ = static_cast<unsigned short>(rawWord4);

                                    int rawWord5;
                                    CheckForEscapeCode(bitsShiftedCounter, rawWord5, rawBitStreamPtr, rawWord4, v3);

                                }

                                SetLoWord(rawWord4, gTbl2[table_index_2].mOutputWord2);
                            } while (!(u16)rawWord4);


                            if ((u16)rawWord4 != 0x7C1F)
                            {
                                break;
                            }

                            OutputWordAndAdvance(rawBitStreamPtr, rawWord4, pOutput, bitsShiftedCounter, v3);
                        } // End while

                        *pOutput++ = static_cast<unsigned short>(rawWord4);

                        if ((u16)rawWord4 == MDEC_END)
                        {
                            const int t11Bits = GetBits(v3, 11);
                            SkipBits(v3, 11, bitsShiftedCounter);

                            if (t11Bits == MASK_10_BITS)
                            {
                                return ret;
                            }

                            rawWord4 = t11Bits & MASK_11_BITS;
                            *pOutput++ = static_cast<unsigned short>(rawWord4);

                            int rawWord7;
                            CheckForEscapeCode(bitsShiftedCounter, rawWord7, rawBitStreamPtr, rawWord4, v3);
                        }

                        SetLoWord(rawWord4, gTbl2[table_index_2].mOutputWord3);

                    } while (!(u16)rawWord4);


                    if ((u16)rawWord4 != 0x7C1F)
                    {
                        break;
                    }


                    OutputWordAndAdvance(rawBitStreamPtr, rawWord4, pOutput, bitsShiftedCounter, v3);
                } // End while

                *pOutput++ = static_cast<unsigned short>(rawWord4);

            } while ((u16)rawWord4 != MDEC_END);

            const int tmp11Bits2 = GetBits(v3, 11);
            SkipBits(v3, 11, bitsShiftedCounter);

            if (tmp11Bits2 == MASK_10_BITS)
            {
                return ret;
            }

            rawWord4 = tmp11Bits2;
            *pOutput++ = static_cast<unsigned short>(rawWord4);

            int rawWord9;
            CheckForEscapeCode(bitsShiftedCounter, rawWord9, rawBitStreamPtr, rawWord4, v3);

        }

        return ret;
    }

    const u32 gQuant1_dword_42AEC8[64] =
    {
        0x0000000C, 0x0000000B, 0x0000000A, 0x0000000C, 0x0000000E, 0x0000000E, 0x0000000D, 0x0000000E,
        0x00000010, 0x00000018, 0x00000013, 0x00000010, 0x00000011, 0x00000012, 0x00000018, 0x00000016,
        0x00000016, 0x00000018, 0x0000001A, 0x00000028, 0x00000033, 0x0000003A, 0x00000028, 0x0000001D,
        0x00000025, 0x00000023, 0x00000031, 0x00000048, 0x00000040, 0x00000037, 0x00000038, 0x00000033,
        0x00000039, 0x0000003C, 0x0000003D, 0x00000037, 0x00000045, 0x00000057, 0x00000044, 0x00000040,
        0x0000004E, 0x0000005C, 0x0000005F, 0x00000057, 0x00000051, 0x0000006D, 0x00000050, 0x00000038,
        0x0000003E, 0x00000067, 0x00000068, 0x00000067, 0x00000062, 0x00000070, 0x00000079, 0x00000071,
        0x0000004D, 0x0000005C, 0x00000078, 0x00000064, 0x00000067, 0x00000065, 0x00000063, 0x00000010
    };

    const u32 gQaunt2_dword_42AFC4[64] =
    {
        0x00000010, 0x00000012, 0x00000012, 0x00000018, 0x00000015, 0x00000018, 0x0000002F, 0x0000001A,
        0x0000001A, 0x0000002F, 0x00000063, 0x00000042, 0x00000038, 0x00000042, 0x00000063, 0x00000063,
        0x00000063, 0x00000063, 0x00000063, 0x00000063, 0x00000063, 0x00000063, 0x00000063, 0x00000063,
        0x00000063, 0x00000063, 0x00000063, 0x00000063, 0x00000063, 0x00000063, 0x00000063, 0x00000063,
        0x00000063, 0x00000063, 0x00000063, 0x00000063, 0x00000063, 0x00000063, 0x00000063, 0x00000063,
        0x00000063, 0x00000063, 0x00000063, 0x00000063, 0x00000063, 0x00000063, 0x00000063, 0x00000063,
        0x00000063, 0x00000063, 0x00000063, 0x00000063, 0x00000063, 0x00000063, 0x00000063, 0x00000063,
        0x00000063, 0x00000063, 0x00000063, 0x00000063, 0x00000063, 0x00000063, 0x00000063, 0x00000063
    };

    const u32 g_block_related_1_dword_42B0C8[64] =
    {
        0x00000001, 0x00000008, 0x00000010, 0x00000009, 0x00000002, 0x00000003, 0x0000000A, 0x00000011,
        0x00000018, 0x00000020, 0x00000019, 0x00000012, 0x0000000B, 0x00000004, 0x00000005, 0x0000000C,
        0x00000013, 0x0000001A, 0x00000021, 0x00000028, 0x00000030, 0x00000029, 0x00000022, 0x0000001B,
        0x00000014, 0x0000000D, 0x00000006, 0x00000007, 0x0000000E, 0x00000015, 0x0000001C, 0x00000023,
        0x0000002A, 0x00000031, 0x00000038, 0x00000039, 0x00000032, 0x0000002B, 0x00000024, 0x0000001D,
        0x00000016, 0x0000000F, 0x00000017, 0x0000001E, 0x00000025, 0x0000002C, 0x00000033, 0x0000003A,
        0x0000003B, 0x00000034, 0x0000002D, 0x00000026, 0x0000001F, 0x00000027, 0x0000002E, 0x00000035,
        0x0000003C, 0x0000003D, 0x00000036, 0x0000002F, 0x00000037, 0x0000003E, 0x0000003F, 0x0000098E // TODO: Last value too large?
    };

    // Same as PSXMDECDecoder::RL_ZSCAN_MATRIX
    const u32 g_block_related_unknown_dword_42B0C4[64] =
    {
        0x00000000, 0x00000001, 0x00000008, 0x00000010, 0x00000009, 0x00000002, 0x00000003, 0x0000000A,
        0x00000011, 0x00000018, 0x00000020, 0x00000019, 0x00000012, 0x0000000B, 0x00000004, 0x00000005,
        0x0000000C, 0x00000013, 0x0000001A, 0x00000021, 0x00000028, 0x00000030, 0x00000029, 0x00000022,
        0x0000001B, 0x00000014, 0x0000000D, 0x00000006, 0x00000007, 0x0000000E, 0x00000015, 0x0000001C,
        0x00000023, 0x0000002A, 0x00000031, 0x00000038, 0x00000039, 0x00000032, 0x0000002B, 0x00000024,
        0x0000001D, 0x00000016, 0x0000000F, 0x00000017, 0x0000001E, 0x00000025, 0x0000002C, 0x00000033,
        0x0000003A, 0x0000003B, 0x00000034, 0x0000002D, 0x00000026, 0x0000001F, 0x00000027, 0x0000002E,
        0x00000035, 0x0000003C, 0x0000003D, 0x00000036, 0x0000002F, 0x00000037, 0x0000003E, 0x0000003F
    };

    const u32 g_block_related_2_dword_42B0CC[64] =
    {
        0x00000008, 0x00000010, 0x00000009, 0x00000002, 0x00000003, 0x0000000A, 0x00000011, 0x00000018,
        0x00000020, 0x00000019, 0x00000012, 0x0000000B, 0x00000004, 0x00000005, 0x0000000C, 0x00000013,
        0x0000001A, 0x00000021, 0x00000028, 0x00000030, 0x00000029, 0x00000022, 0x0000001B, 0x00000014,
        0x0000000D, 0x00000006, 0x00000007, 0x0000000E, 0x00000015, 0x0000001C, 0x00000023, 0x0000002A,
        0x00000031, 0x00000038, 0x00000039, 0x00000032, 0x0000002B, 0x00000024, 0x0000001D, 0x00000016,
        0x0000000F, 0x00000017, 0x0000001E, 0x00000025, 0x0000002C, 0x00000033, 0x0000003A, 0x0000003B,
        0x00000034, 0x0000002D, 0x00000026, 0x0000001F, 0x00000027, 0x0000002E, 0x00000035, 0x0000003C,
        0x0000003D, 0x00000036, 0x0000002F, 0x00000037, 0x0000003E, 0x0000003F, 0x0000098E, 0x0000098E
    };

    const u32 g_block_related_3_dword_42B0D0[64] =
    {
        0x00000010, 0x00000009, 0x00000002, 0x00000003, 0x0000000A, 0x00000011, 0x00000018, 0x00000020,
        0x00000019, 0x00000012, 0x0000000B, 0x00000004, 0x00000005, 0x0000000C, 0x00000013, 0x0000001A,
        0x00000021, 0x00000028, 0x00000030, 0x00000029, 0x00000022, 0x0000001B, 0x00000014, 0x0000000D,
        0x00000006, 0x00000007, 0x0000000E, 0x00000015, 0x0000001C, 0x00000023, 0x0000002A, 0x00000031,
        0x00000038, 0x00000039, 0x00000032, 0x0000002B, 0x00000024, 0x0000001D, 0x00000016, 0x0000000F,
        0x00000017, 0x0000001E, 0x00000025, 0x0000002C, 0x00000033, 0x0000003A, 0x0000003B, 0x00000034,
        0x0000002D, 0x00000026, 0x0000001F, 0x00000027, 0x0000002E, 0x00000035, 0x0000003C, 0x0000003D,
        0x00000036, 0x0000002F, 0x00000037, 0x0000003E, 0x0000003F, 0x0000098E, 0x0000098E, 0x0000F384
    };

    u32 g_252_buffer_unk_635A0C[64] = {};
    u32 g_252_buffer_unk_63580C[64] = {};

    // Return val becomes param 1
    int16_t* ddv_func7_DecodeMacroBlock_impl(int16_t* bitstreamPtr, int16_t* outputBlockPtr, bool isYBlock)
    {
        int v1; // ebx@1
        u32 *v2; // esi@1
        u16* endPtr; // edx@3
        u32 *output_q; // ebp@3
        unsigned int counter; // edi@3
        u32* v6; // esi@3
        u16* outptr; // edx@3
        u16* dataPtr; // edx@5
        unsigned int macroBlockWord; // eax@6
        unsigned int blockNumberQ; // edi@9
        int index1; // eax@14
        int index2; // esi@14
        int index3; // ecx@14
        signed int v14; // eax@15
        int cnt; // ecx@15
        unsigned int v16; // ecx@15
        // int v17; // esi@15
        int idx; // ebx@16
        u32 outVal = 0; // ecx@18
        unsigned int macroBlockWord1; // eax@20
        //  int v21; // esi@21
        unsigned int v22; // edi@21
        int v23; // ebx@21
        //   signed int v24; // eax@21
        u32 v25 = 0; // ecx@21
        // DecodeMacroBlock_Struct *thisPtr; // [sp-4h] [bp-10h]@3

        v1 = isYBlock /*this->ZeroOrOneConstant*/;                 // off 14
        v2 = &g_252_buffer_unk_63580C[1];

        if (!isYBlock /*this->ZeroOrOneConstant*/)
        {
            v2 = &g_252_buffer_unk_635A0C[1];
        }

        v6 = v2;
        counter = 0;
        outptr = (u16*)bitstreamPtr /*this->mOutput >> 1*/;
        //thisPtr = this;
        output_q = (u32*)outputBlockPtr /*this->mCoEffsBuffer*/;               // off 10 quantised coefficients
        endPtr = outptr - 1;

        do
        {
            ++endPtr;
        } while (*endPtr == 0xFE00u);  // 0xFE00 == END_OF_BLOCK, hence this loop moves past the EOB

        *output_q = (v1 << 10) + 2 * (*endPtr << 21 >> 22);
        dataPtr = endPtr + 1; // last use of endPtr


        if (*(u8 *)(dataPtr - 1) & 1)        // has video flag?
        {

            do
            {
                macroBlockWord1 = *dataPtr++;// bail if end
                if (macroBlockWord1 == 0xFE00)
                {
                    break;
                }
                u32* v21 = (macroBlockWord1 >> 10) + v6;
                v22 = (macroBlockWord1 >> 10) + counter;
                v23 = g_block_related_1_dword_42B0C8[v22];
                signed int v24 = output_q[v23] + (macroBlockWord1 << 22);
                SetHiWord(v25, GetHiWord(v24));
                counter = v22 + 1;
                SetLoWord(v25, static_cast<u16>((*(v21)* (v24 >> 22) + 4) >> 3));
                v6 = v21 + 1;
                output_q[v23] = v25;
            } while (counter < 63);                     // 63 AC values?

        }
        else
        {

            while (1)
            {
                macroBlockWord = *dataPtr++;// bail if end
                if (macroBlockWord == 0xFE00)
                {
                    break;
                }
                v16 = macroBlockWord;
                v14 = macroBlockWord << 22;
                v16 >>= 10;
                u32* v17 = v16 + v6;
                cnt = v16 + 1;
                while (1)
                {
                    --cnt;
                    idx = g_block_related_1_dword_42B0C8[counter];
                    if (!cnt)
                    {
                        break;
                    }
                    output_q[idx] = 0;
                    ++counter;
                }
                SetHiWord(outVal, GetHiWord(v14));
                ++counter;
                SetLoWord(outVal, static_cast<u16>((*(v17)* (v14 >> 22) + 4) >> 3));
                v6 = v17 + 1;
                output_q[idx] = outVal;
                if (counter >= 63)                      // 63 AC values?
                {
                    return (int16_t*)dataPtr;
                }
            }
            if (counter)
            {
                blockNumberQ = counter + 1;
                
                if (blockNumberQ & 3)
                {
                    //blockNumberQ++;
                    output_q[g_block_related_unknown_dword_42B0C4[blockNumberQ++]] = 0;
                    if (blockNumberQ & 3)
                    {
                       // blockNumberQ++;
                        output_q[g_block_related_unknown_dword_42B0C4[blockNumberQ++]] = 0;
                        if (blockNumberQ & 3)
                        {
                           // blockNumberQ++;
                            output_q[g_block_related_unknown_dword_42B0C4[blockNumberQ++]] = 0;
                        }
                    }
                }
                

                while (blockNumberQ != 64)              // 63 AC values?
                {
                    index1 = g_block_related_1_dword_42B0C8[blockNumberQ];
                    index2 = g_block_related_2_dword_42B0CC[blockNumberQ];
                    index3 = g_block_related_3_dword_42B0D0[blockNumberQ];
                    output_q[g_block_related_unknown_dword_42B0C4[blockNumberQ]] = 0;
                    output_q[index1] = 0;
                    output_q[index2] = 0;
                    output_q[index3] = 0;
                    blockNumberQ += 4;
                    //blockNumberQ++;
                }
            }
            else
            {
                memset(output_q + 1, 0, 252u);            // 63 dwords buffer
            }

        }
        return (int16_t*)dataPtr;
    }

    // TODO: Should probably just be 64? Making this bigger fixes a sound glitch which is probably caused
    // by an out of bounds write somewhere.
    typedef std::array<int32_t, 64 * 4> T64IntsArray;

    static T64IntsArray Cr_block = {};
    static T64IntsArray Cb_block = {};
    static T64IntsArray Y1_block = {};
    static T64IntsArray Y2_block = {};
    static T64IntsArray Y3_block = {};
    static T64IntsArray Y4_block = {};


    void half_idct(T64IntsArray& pSource, T64IntsArray& pDestination, int nPitch, int nIncrement, int nShift)
    {
        std::array<int32_t, 8> pTemp;

        size_t sourceIdx = 0;
        size_t destinationIdx = 0;

        for (int i = 0; i < 8; i++)
        {
            pTemp[4] = pSource[(0 * nPitch) + sourceIdx] * 8192 + pSource[(2 * nPitch) + sourceIdx] * 10703 + pSource[(4 * nPitch) + sourceIdx] * 8192 + pSource[(6 * nPitch) + sourceIdx] * 4433;
            pTemp[5] = pSource[(0 * nPitch) + sourceIdx] * 8192 + pSource[(2 * nPitch) + sourceIdx] * 4433 - pSource[(4 * nPitch) + sourceIdx] * 8192 - pSource[(6 * nPitch) + sourceIdx] * 10704;
            pTemp[6] = pSource[(0 * nPitch) + sourceIdx] * 8192 - pSource[(2 * nPitch) + sourceIdx] * 4433 - pSource[(4 * nPitch) + sourceIdx] * 8192 + pSource[(6 * nPitch) + sourceIdx] * 10704;
            pTemp[7] = pSource[(0 * nPitch) + sourceIdx] * 8192 - pSource[(2 * nPitch) + sourceIdx] * 10703 + pSource[(4 * nPitch) + sourceIdx] * 8192 - pSource[(6 * nPitch) + sourceIdx] * 4433;

            pTemp[0] = pSource[(1 * nPitch) + sourceIdx] * 11363 + pSource[(3 * nPitch) + sourceIdx] * 9633 + pSource[(5 * nPitch) + sourceIdx] * 6437 + pSource[(7 * nPitch) + sourceIdx] * 2260;
            pTemp[1] = pSource[(1 * nPitch) + sourceIdx] * 9633 - pSource[(3 * nPitch) + sourceIdx] * 2259 - pSource[(5 * nPitch) + sourceIdx] * 11362 - pSource[(7 * nPitch) + sourceIdx] * 6436;
            pTemp[2] = pSource[(1 * nPitch) + sourceIdx] * 6437 - pSource[(3 * nPitch) + sourceIdx] * 11362 + pSource[(5 * nPitch) + sourceIdx] * 2261 + pSource[(7 * nPitch) + sourceIdx] * 9633;
            pTemp[3] = pSource[(1 * nPitch) + sourceIdx] * 2260 - pSource[(3 * nPitch) + sourceIdx] * 6436 + pSource[(5 * nPitch) + sourceIdx] * 9633 - pSource[(7 * nPitch) + sourceIdx] * 11363;

            pDestination[(0 * nPitch) + destinationIdx] = (pTemp[4] + pTemp[0]) >> nShift;
            pDestination[(1 * nPitch) + destinationIdx] = (pTemp[5] + pTemp[1]) >> nShift;
            pDestination[(2 * nPitch) + destinationIdx] = (pTemp[6] + pTemp[2]) >> nShift;
            pDestination[(3 * nPitch) + destinationIdx] = (pTemp[7] + pTemp[3]) >> nShift;
            pDestination[(4 * nPitch) + destinationIdx] = (pTemp[7] - pTemp[3]) >> nShift;
            pDestination[(5 * nPitch) + destinationIdx] = (pTemp[6] - pTemp[2]) >> nShift;
            pDestination[(6 * nPitch) + destinationIdx] = (pTemp[5] - pTemp[1]) >> nShift;
            pDestination[(7 * nPitch) + destinationIdx] = (pTemp[4] - pTemp[0]) >> nShift;

            sourceIdx += nIncrement;
            destinationIdx += nIncrement;
        }
    }

    // 0x40ED90
    void idct(int16_t* input, T64IntsArray& pDestination) // dst is 64 dwords
    {
        T64IntsArray pTemp;
        T64IntsArray pExtendedSource;

        // Source is passed as signed 16 bits stored every 32 bits
        // We sign extend it at the beginning like Masher does
        for (int i = 0; i < 64; i++)
        {
            pExtendedSource[i] = input[i * 2];
        }

        half_idct(pExtendedSource, pTemp, 8, 1, 11);
        half_idct(pTemp, pDestination, 1, 8, 18);
    }

    static int To1d(int x, int y)
    {
        // 8x8 index to x64 index
        return y * 8 + x;
    }

    unsigned char Clamp(f32 v)
    {
        if (v < 0.0f) v = 0.0f;
        if (v > 255.0f) v = 255.0f;
        return (unsigned char)v;
    }

    void SetElement(int x, int y, int width, u32* ptr, u32 value)
    {
        ptr[(width * y) + x] = value;
    }

    static void ConvertYuvToRgbAndBlit(u32* pixelBuffer, int xoff, int yoff, int width, int height)
    {
        // convert the Y1 Y2 Y3 Y4 and Cb and Cr blocks into a 16x16 array of (Y, Cb, Cr) pixels
        struct Macroblock_YCbCr_Struct
        {
            f32 Y;
            f32 Cb;
            f32 Cr;
        };

        std::array< std::array<Macroblock_YCbCr_Struct, 16>, 16> Macroblock_YCbCr = {};

        for (int x = 0; x < 8; x++)
        {
            for (int y = 0; y < 8; y++)
            {
                Macroblock_YCbCr[x][y].Y = static_cast<f32>(Y1_block[To1d(x, y)]);
                Macroblock_YCbCr[x + 8][y].Y = static_cast<f32>(Y2_block[To1d(x, y)]);
                Macroblock_YCbCr[x][y + 8].Y = static_cast<f32>(Y3_block[To1d(x, y)]);
                Macroblock_YCbCr[x + 8][y + 8].Y = static_cast<f32>(Y4_block[To1d(x, y)]);

                Macroblock_YCbCr[x * 2][y * 2].Cb = static_cast<f32>(Cb_block[To1d(x, y)]);
                Macroblock_YCbCr[x * 2 + 1][y * 2].Cb = static_cast<f32>(Cb_block[To1d(x, y)]);
                Macroblock_YCbCr[x * 2][y * 2 + 1].Cb = static_cast<f32>(Cb_block[To1d(x, y)]);
                Macroblock_YCbCr[x * 2 + 1][y * 2 + 1].Cb = static_cast<f32>(Cb_block[To1d(x, y)]);

                Macroblock_YCbCr[x * 2][y * 2].Cr = static_cast<f32>(Cr_block[To1d(x, y)]);
                Macroblock_YCbCr[x * 2 + 1][y * 2].Cr = static_cast<f32>(Cr_block[To1d(x, y)]);
                Macroblock_YCbCr[x * 2][y * 2 + 1].Cr = static_cast<f32>(Cr_block[To1d(x, y)]);
                Macroblock_YCbCr[x * 2 + 1][y * 2 + 1].Cr = static_cast<f32>(Cr_block[To1d(x, y)]);
            }
        }

        // Convert the (Y, Cb, Cr) pixels into RGB pixels
        struct Macroblock_RGB_Struct
        {
            unsigned char Red;
            unsigned char Green;
            unsigned char Blue;
            unsigned char A;
        };

        std::array< std::array<Macroblock_RGB_Struct, 16>, 16> Macroblock_RGB = {};

        for (int x = 0; x < 16; x++)
        {
            for (int y = 0; y < 16; y++)
            {
                const f32 r = (Macroblock_YCbCr[x][y].Y) + 1.402f *  Macroblock_YCbCr[x][y].Cb;
                const f32 g = (Macroblock_YCbCr[x][y].Y) - 0.3437f * Macroblock_YCbCr[x][y].Cr - 0.7143f * Macroblock_YCbCr[x][y].Cb;
                const f32 b = (Macroblock_YCbCr[x][y].Y) + 1.772f *  Macroblock_YCbCr[x][y].Cr;

                Macroblock_RGB[x][y].Red = Clamp(r);
                Macroblock_RGB[x][y].Green = Clamp(g);
                Macroblock_RGB[x][y].Blue = Clamp(b);

                // Due to macro block padding this can be out of bounds
                int xpos = x + xoff;
                int ypos = y + yoff;
                if (xpos < width && ypos < height)
                {
                    u32 pixelValue = 0;
                    pixelValue = (pixelValue << 8) + Macroblock_RGB[x][y].Blue;
                    pixelValue = (pixelValue << 8) + Macroblock_RGB[x][y].Green;
                    pixelValue = (pixelValue << 8) + Macroblock_RGB[x][y].Red;

                    // Actually is no alpha in FMVs
                    // pixelValue = (pixelValue << 8) + Macroblock_RGB[x][y].A
                    SetElement(xpos, ypos, width, pixelBuffer, pixelValue);
                }
            }
        }
    }

    static void after_block_decode_no_effect_q_impl(int quantScale)
    {
        g_252_buffer_unk_63580C[0] = 16;
        g_252_buffer_unk_635A0C[0] = 16;
        if (quantScale > 0)
        {
            signed int result = 0;
            do
            {
                auto val = gQuant1_dword_42AEC8[result];
                result++;
                g_252_buffer_unk_63580C[result] = quantScale * val;
                g_252_buffer_unk_635A0C[result] = quantScale * gQaunt2_dword_42AFC4[result];


            } while (result < 63);                   // 252/4=63
        }
        else
        {
            // These are simply null buffers to start with
            for (int i = 0; i < 64; i++)
            {
                g_252_buffer_unk_635A0C[i] = 16;
                g_252_buffer_unk_63580C[i] = 16;
            }
            // memset(&g_252_buffer_unk_635A0C[1], 16, 252  /*sizeof(g_252_buffer_unk_635A0C)*/); // u32[63]
            // memset(&g_252_buffer_unk_63580C[1], 16, 252 /*sizeof(g_252_buffer_unk_63580C)*/);
        }

    }

    void Masher::ParseVideoFrame(u32* pixelBuffer)
    {
        if (mNumMacroblocksX <= 0 || mNumMacroblocksY <= 0)
        {
            return;
        }

        const int quantScale = decode_bitstream((u16*)mVideoFrameData.data(), mDecodedVideoFrameData.data());

        after_block_decode_no_effect_q_impl(quantScale);


        int16_t* bitstreamCurPos = (int16_t*)mDecodedVideoFrameData.data();
        int16_t* block1Output = (int16_t*)mMacroBlockBuffer.data();

        int xoff = 0;
        for (unsigned int xBlock = 0; xBlock < mNumMacroblocksX; xBlock++)
        {
            int yoff = 0;
            for (unsigned int yBlock = 0; yBlock < mNumMacroblocksY; yBlock++)
            {
                const int dataSizeBytes = 64 * 4;// thisPtr->mBlockDataSize_q * 4; // Convert to byte count 64*4=256

                int16_t* afterBlock1Ptr = ddv_func7_DecodeMacroBlock_impl(bitstreamCurPos, block1Output, 0);
                idct(block1Output, Cr_block);
                int16_t* block2Output = dataSizeBytes + block1Output;

                int16_t* afterBlock2Ptr = ddv_func7_DecodeMacroBlock_impl(afterBlock1Ptr, block2Output, 0);
                idct(block2Output, Cb_block);
                int16_t* block3Output = dataSizeBytes + block2Output;

                int16_t* afterBlock3Ptr = ddv_func7_DecodeMacroBlock_impl(afterBlock2Ptr, block3Output, 1);
                idct(block3Output, Y1_block);
                int16_t* block4Output = dataSizeBytes + block3Output;

                int16_t* afterBlock4Ptr = ddv_func7_DecodeMacroBlock_impl(afterBlock3Ptr, block4Output, 1);
                idct(block4Output, Y2_block);
                int16_t* block5Output = dataSizeBytes + block4Output;

                int16_t* afterBlock5Ptr = ddv_func7_DecodeMacroBlock_impl(afterBlock4Ptr, block5Output, 1);
                idct(block5Output, Y3_block);
                int16_t* block6Output = dataSizeBytes + block5Output;

                bitstreamCurPos = ddv_func7_DecodeMacroBlock_impl(afterBlock5Ptr, block6Output, 1);
                idct(block6Output, Y4_block);
                block1Output = dataSizeBytes + block6Output;

                ConvertYuvToRgbAndBlit(pixelBuffer, xoff, yoff, mVideoHeader.mWidth, mVideoHeader.mHeight);

                yoff += 16;
            }
            xoff += 16;
        }

    }

    AudioDecompressor::AudioDecompressor()
    {
        init_Snd_tbl();
    }

    /*static*/ s32 AudioDecompressor::GetSoundTableValue(s16 tblIndex)
    {
        const s32 positiveTblIdx = static_cast<s32>(abs(tblIndex));
        const u32 shiftedIdx = (positiveTblIdx >> 7) & 0xFF;
        s32 result = (u16)((s16)gSndTbl_byte_62EEB0[shiftedIdx] << 7) | (u16)(positiveTblIdx >> gSndTbl_byte_62EEB0[shiftedIdx]);
        if (tblIndex < 0)
        {
            result = -result;
        }
        return result;
    }

    s16 AudioDecompressor::sub_408F50(s16 sample)
    {
        s32 absSample = static_cast<s32>(abs(sample));
        s32 sampleBits = absSample >> 7;
        s32 sampleMasked = absSample & 0x7F;

        s16 result = (u16)(sampleMasked << sampleBits);
        if (sampleBits >= 2)
        {
            result |= (u16)(1 << (sampleBits - 2));
        }

        if (sample < 0)
        {
            result = -result;
        }
        return result;
    }

    s32 AudioDecompressor::ReadNextAudioWord(s32 value)
    {
        if (mUsedBits <= 16)
        {
            const int srcVal = *mAudioFrameDataPtr;
            ++mAudioFrameDataPtr;
            value |= srcVal << mUsedBits;
            mUsedBits += 16;
        }
        return value;
    }

    s32 AudioDecompressor::SndRelated_sub_409650()
    {
        const s32 numBits = mUsedBits & 7;
        mUsedBits -= numBits;
        mWorkBits >>= numBits;
        mWorkBits = ReadNextAudioWord(mWorkBits);
        return mUsedBits;
    }

    s16 AudioDecompressor::NextSoundBits(u16 numBits)
    {
        mUsedBits -= numBits;
        const s16 ret = static_cast<s16>(mWorkBits & ((1 << numBits) - 1));
        mWorkBits >>= numBits;
        mWorkBits = ReadNextAudioWord(mWorkBits);
        return ret;
    }

    bool AudioDecompressor::SampleMatches(s16& sample, s16 bitNum)
    {
        const s32 bitMask = 1 << (bitNum - 1);
        if (sample != bitMask)
        {
            if (sample & bitMask)
            {
                sample = -(sample & ~bitMask);
            }
            return true;
        }
        return false;
    }

    void AudioDecompressor::decode_16bit_audio_frame(u16* outPtr, s32 numSamplesPerFrame, bool isLast)
    {
        const s16 useTableFlag = NextSoundBits(16);
        const s16 firstWord = NextSoundBits(16);
        const s16 secondWord = NextSoundBits(16);
        const s16 thirdWord = NextSoundBits(16);

        const s16 previous1 = NextSoundBits(16);
        s32 previousValue1 = static_cast<s16>(previous1);

        *outPtr = previous1;
        outPtr += mAudioFrameSizeBytes;

        const s16 previous2 = NextSoundBits(16);
        s32 previousValue2 = static_cast<s16>(previous2);

        *outPtr = previous2;
        outPtr += mAudioFrameSizeBytes;

        const s16 previous3 = NextSoundBits(16);
        s32 previousValue3 = static_cast<s16>(previous3);

        *outPtr = previous3;
        outPtr += mAudioFrameSizeBytes;

        if (numSamplesPerFrame > 3)
        {
            for (s32 counter = 0; counter < numSamplesPerFrame - 3; counter++)
            {
                s16 samplePart = 0;
                do
                {
                    samplePart = NextSoundBits(firstWord);
                    if (SampleMatches(samplePart, firstWord))
                    {
                        break;
                    }

                    samplePart = NextSoundBits(secondWord);
                    if (SampleMatches(samplePart, secondWord))
                    {
                        break;
                    }

                    samplePart = NextSoundBits(thirdWord);
                    if (SampleMatches(samplePart, thirdWord))
                    {
                        break;
                    }

                } while (false);

                const s32 previous = (5 * previousValue3) - (4 * previousValue2);
                const s32 samplePartOrTableIndex = (previousValue1 + previous) >> 1;

                previousValue1 = previousValue2;
                previousValue2 = previousValue3;

                const bool bUseTbl = useTableFlag != 0;
                if (bUseTbl)
                {
                    const s32 soundTableValue = GetSoundTableValue(static_cast<s16>(samplePartOrTableIndex));
                    previousValue3 = sub_408F50(static_cast<s16>(samplePart + soundTableValue));
                }
                else
                {
                    // TODO: Case never hit for any known data?
                    previousValue3 = static_cast<s16>(samplePartOrTableIndex + samplePart);
                }

                *outPtr = static_cast<u16>(previousValue3); // int to word
                outPtr += mAudioFrameSizeBytes;
            }
        }

        if (!isLast)
        {
            SndRelated_sub_409650();
        }
    }

    u16* AudioDecompressor::SetupAudioDecodePtrs(u16 *rawFrameBuffer)
    {
        mAudioFrameDataPtr = rawFrameBuffer;
        mWorkBits = *(u32 *)mAudioFrameDataPtr;
        mAudioFrameDataPtr = mAudioFrameDataPtr + 2;
        mUsedBits = 32;
        return mAudioFrameDataPtr;
    }

    s32 AudioDecompressor::SetAudioFrameSizeBytesAndBits(s32 audioFrameSizeBytes)
    {
        mAudioFrameSizeBytes = audioFrameSizeBytes;
        return mAudioFrameSizeBytes;
    }

    /*static*/ void AudioDecompressor::init_Snd_tbl()
    {
        static bool done = false;
        if (!done)
        {
            done = true;
            int index = 0;
            do
            {
                int tableValue = 0;
                for (int i = index; i > 0; ++tableValue)
                {
                    i >>= 1;
                }
                gSndTbl_byte_62EEB0[index++] = static_cast<u8>(tableValue);
            } while (index < 256);
        }
    }


    /*static*/ u8 AudioDecompressor::gSndTbl_byte_62EEB0[256];

    void Masher::decode_audio_frame(u16 *rawFrameBuffer, u16 *outPtr, signed int numSamplesPerFrame)
    {
        AudioDecompressor decompressor;
        decompressor.SetAudioFrameSizeBytesAndBits(2);
        decompressor.SetupAudioDecodePtrs(rawFrameBuffer);
        memset(outPtr, 0, numSamplesPerFrame * 4);
        decompressor.decode_16bit_audio_frame(outPtr, numSamplesPerFrame, false);

        if (decompressor.mAudioFrameSizeBytes == 2)
        {
            decompressor.decode_16bit_audio_frame(outPtr + 1, numSamplesPerFrame, true);
        }
    }

    // 0040DBB0
    void Masher::do_decode_audio_frame(u8* audioBuffer)
    {
        if (mbHasAudio)
        {
            decode_audio_frame((u16 *)mAudioFrameData.data(), (u16 *)audioBuffer, mAudioHeader.mSingleAudioFrameSize);
        }
    }

    void Masher::ParseAudioFrame(u8* audioBuffer)
    {
        if (audioBuffer)
        {
            do_decode_audio_frame(audioBuffer);
        }
    }

    bool Masher::Update(u32* pixelBuffer, u8* audioBuffer)
    {
        if (mCurrentFrame < mFileHeader.mNumberOfFrames)
        {
            if (mbHasVideo && mbHasAudio)
            {
                // If there is video and audio then the first dword is
                // the size of the video data, and the audio data is
                // the remaining data after this.
                uint32_t videoDataSize = 0;
                mStream->Read(videoDataSize);

                // Video data
                mVideoFrameData.resize(videoDataSize);
                mStream->Read(mVideoFrameData);

                // Calc size of audio data
                const uint32_t totalSize = mFrameSizes[mCurrentFrame];
                const uint32_t audioDataSize = totalSize - videoDataSize;

                // Audio data
                mAudioFrameData.resize(audioDataSize);
                mStream->Read(mAudioFrameData);
                ParseVideoFrame(pixelBuffer);
                ParseAudioFrame(audioBuffer);
            }
            else if (mbHasAudio)
            {
                const uint32_t totalSize = mFrameSizes[mCurrentFrame];
                mAudioFrameData.resize(totalSize + 4); // TODO: Figure out if this is required or is just a bug
                mStream->ReadBytes(mAudioFrameData.data(), totalSize);
                ParseAudioFrame(audioBuffer);
            }
            else if (mbHasVideo)
            {
                const uint32_t totalSize = mFrameSizes[mCurrentFrame];
                mVideoFrameData.resize(totalSize);
                mStream->Read(mVideoFrameData);
                ParseVideoFrame(pixelBuffer);
            }
            mCurrentFrame++;
            return true;
        }
        return false;
    }
}
