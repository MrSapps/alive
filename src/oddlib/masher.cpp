#include "oddlib/masher.hpp"
#include "oddlib/exceptions.hpp"
#include "oddlib/lvlarchive.hpp"
#include "oddlib/masher_tables.hpp"
#include "logger.hpp"
#include <assert.h>
#include <array>

namespace Oddlib
{
    void Masher::Read()
    {
        mStream.ReadUInt32(mFileHeader.mDdvTag);
        if (mFileHeader.mDdvTag != MakeType('D', 'D', 'V', 0))
        {
            LOG_ERROR("Invalid DDV magic tag " << mFileHeader.mDdvTag);
            throw Exception("Invalid DDV tag");
        }

        mStream.ReadUInt32(mFileHeader.mDdvVersion);
        if (mFileHeader.mDdvVersion != 1)
        {
            // This is the only version seen in all of the known data
            LOG_ERROR("Expected DDV version to be 2 but got " << mFileHeader.mDdvVersion);
            throw Exception("Wrong DDV version");
        }

        mStream.ReadUInt32(mFileHeader.mContains);
        mStream.ReadUInt32(mFileHeader.mFrameRate);
        mStream.ReadUInt32(mFileHeader.mNumberOfFrames);

        mbHasVideo = (mFileHeader.mContains & 0x1) == 0x1;
        mbHasAudio = (mFileHeader.mContains & 0x2) == 0x2;

        if (mbHasVideo)
        {
            mStream.ReadUInt32(mVideoHeader.mUnknown);
            mStream.ReadUInt32(mVideoHeader.mWidth);
            mStream.ReadUInt32(mVideoHeader.mHeight); 
            mStream.ReadUInt32(mVideoHeader.mMaxAudioFrameSize);
            mStream.ReadUInt32(mVideoHeader.mMaxVideoFrameSize);
            mStream.ReadUInt32(mVideoHeader.mKeyFrameRate);

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
            mStream.ReadUInt32(mAudioHeader.mAudioFormat);
            mStream.ReadUInt32(mAudioHeader.mSampleRate);
            mStream.ReadUInt32(mAudioHeader.mMaxAudioFrameSize);
            mStream.ReadUInt32(mAudioHeader.mSingleAudioFrameSize);
            mStream.ReadUInt32(mAudioHeader.mNumberOfFramesInterleave);
            

            for (uint32_t i = 0; i < mAudioHeader.mNumberOfFramesInterleave; i++)
            {
                uint32_t tmp = 0;
                mStream.ReadUInt32(tmp);
                mAudioFrameSizes.emplace_back(tmp);
            }
        }

        for (uint32_t i = 0; i < mFileHeader.mNumberOfFrames; i++)
        {
            uint32_t tmp = 0;
            mStream.ReadUInt32(tmp);
            mFrameSizes.emplace_back(tmp);
        }
        
        // TODO: Read/skip mAudioHeader.mNumberOfFramesInterleave frame datas

        mMacroBlockBuffer.resize(mNumMacroblocksX * mNumMacroblocksY * mVideoHeader.mMaxVideoFrameSize);

        mDecodedVideoFrameData.resize(mVideoHeader.mMaxVideoFrameSize);

        // TODO: Need work buffer for audio
        mAudioFrameData.resize(mVideoHeader.mMaxAudioFrameSize);
    }

    static void SetLoWord(Uint32& v, Uint16 lo)
    {
        Uint16 hiWord = v & 0xFFFF;
        v = (lo & 0xFFFF) | (hiWord << 16);
    }

    static Uint16 GetHiWord(Uint32 v)
    {
        return static_cast<Uint16>((v >> 16) & 0xFFFF);
    }

    static void SetHiWord(Uint32& v, Uint16 hi)
    {
        Uint16 tmp = v & 0xffff;
        v = (tmp) | ((hi & 0xffff) << 16);
    }

    static inline void CheckForEscapeCode(char& bitsToShiftBy, int& rawWord1, Uint16*& rawBitStreamPtr, Uint32& rawWord4, Uint32& v25)
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

    static inline void OutputWordAndAdvance(Uint16*& rawBitStreamPtr, Uint32& rawWord4, unsigned short int*& pOut, char& numBitsToShiftBy, Uint32& v3)
    {
        *pOut++ = v3 >> (32 - 16);

        rawWord4 = *rawBitStreamPtr++ << numBitsToShiftBy;
        v3 = rawWord4 | (v3 << 16);
    }

#define MASK_11_BITS 0x7FF
#define MASK_10_BITS 0x3FF
#define MASK_13_BITS 0x1FFF
#define MDEC_END 0xFE00u

    static Uint32 GetBits(Uint32 value, Uint32 numBits)
    {
        return value >> (32 - numBits);
    }

    static void SkipBits(Uint32& value, char numBits, char& bitPosCounter)
    {
        value = value << numBits;
        bitPosCounter += numBits;
    }

    int decode_bitstream(Uint16 *pFrameData, unsigned short int *pOutput)
    {

        unsigned int table_index_2 = 0;
        int ret = *pFrameData;
        Uint32 v8 = *(Uint32*)(pFrameData + 1);
        Uint16* rawBitStreamPtr = (pFrameData + 3);

        v8 = (v8 << 16) | (v8 >> 16); // Swap words

        Uint32 rawWord4 = GetBits(v8, 11);

        char bitsShiftedCounter = 0;
        SkipBits(v8, 11, bitsShiftedCounter);
        Uint32 v3 = v8;

        *pOutput++ = rawWord4; // store in output

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

                                    if ((Uint16)rawWord4 != 0x7C1F) // 0b 11111 00000 11111
                                    {
                                        break;
                                    }

                                    OutputWordAndAdvance(rawBitStreamPtr, rawWord4, pOutput, bitsShiftedCounter, v3);
                                } // End while

                                *pOutput++ = rawWord4;

                                if ((Uint16)rawWord4 == MDEC_END)
                                {
                                    const int v15 = GetBits(v3, 11);
                                    SkipBits(v3, 11, bitsShiftedCounter);

                                    if (v15 == MASK_10_BITS)
                                    {
                                        return ret;
                                    }

                                    rawWord4 = v15 & MASK_11_BITS;
                                    *pOutput++ = rawWord4;

                                    int rawWord5;
                                    CheckForEscapeCode(bitsShiftedCounter, rawWord5, rawBitStreamPtr, rawWord4, v3);

                                }

                                SetLoWord(rawWord4, gTbl2[table_index_2].mOutputWord2);
                            } while (!(Uint16)rawWord4);


                            if ((Uint16)rawWord4 != 0x7C1F)
                            {
                                break;
                            }

                            OutputWordAndAdvance(rawBitStreamPtr, rawWord4, pOutput, bitsShiftedCounter, v3);
                        } // End while

                        *pOutput++ = rawWord4;

                        if ((Uint16)rawWord4 == MDEC_END)
                        {
                            const int t11Bits = GetBits(v3, 11);
                            SkipBits(v3, 11, bitsShiftedCounter);

                            if (t11Bits == MASK_10_BITS)
                            {
                                return ret;
                            }

                            rawWord4 = t11Bits & MASK_11_BITS;
                            *pOutput++ = rawWord4;

                            int rawWord7;
                            CheckForEscapeCode(bitsShiftedCounter, rawWord7, rawBitStreamPtr, rawWord4, v3);
                        }

                        SetLoWord(rawWord4, gTbl2[table_index_2].mOutputWord3);

                    } while (!(Uint16)rawWord4);


                    if ((Uint16)rawWord4 != 0x7C1F)
                    {
                        break;
                    }


                    OutputWordAndAdvance(rawBitStreamPtr, rawWord4, pOutput, bitsShiftedCounter, v3);
                } // End while

                *pOutput++ = rawWord4;

            } while ((Uint16)rawWord4 != MDEC_END);

            const int tmp11Bits2 = GetBits(v3, 11);
            SkipBits(v3, 11, bitsShiftedCounter);

            if (tmp11Bits2 == MASK_10_BITS)
            {
                return ret;
            }

            rawWord4 = tmp11Bits2;
            *pOutput++ = rawWord4;

            int rawWord9;
            CheckForEscapeCode(bitsShiftedCounter, rawWord9, rawBitStreamPtr, rawWord4, v3);

        }

        return ret;
    }
    
    const Uint32 gQuant1_dword_42AEC8[64] =
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

    const Uint32 gQaunt2_dword_42AFC4[64] =
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

    const Uint32 g_block_related_1_dword_42B0C8[64] =
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

    const Uint32 g_block_related_unknown_dword_42B0C4[64] =
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

    const Uint32 g_block_related_2_dword_42B0CC[64] =
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

    const Uint32 g_block_related_3_dword_42B0D0[64] =
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

    Uint32 g_252_buffer_unk_635A0C[64] = {};
    Uint32 g_252_buffer_unk_63580C[64] = {};

    // Return val becomes param 1
    int16_t* __cdecl ddv_func7_DecodeMacroBlock_impl(int16_t* bitstreamPtr, int16_t* outputBlockPtr, bool isYBlock)
    {
        int v1; // ebx@1
        Uint32 *v2; // esi@1
        Uint16* endPtr; // edx@3
        Uint32 *output_q; // ebp@3
        unsigned int counter; // edi@3
        Uint32* v6; // esi@3
        Uint16* outptr; // edx@3
        Uint16* dataPtr; // edx@5
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
        Uint32 outVal; // ecx@18
        unsigned int macroBlockWord1; // eax@20
        //  int v21; // esi@21
        unsigned int v22; // edi@21
        int v23; // ebx@21
        //   signed int v24; // eax@21
        Uint32 v25; // ecx@21
        // DecodeMacroBlock_Struct *thisPtr; // [sp-4h] [bp-10h]@3

        v1 = isYBlock /*this->ZeroOrOneConstant*/;                 // off 14
        v2 = &g_252_buffer_unk_63580C[1];

        if (!isYBlock /*this->ZeroOrOneConstant*/)
        {
            v2 = &g_252_buffer_unk_635A0C[1];
        }

        v6 = v2;
        counter = 0;
        outptr = (Uint16*)bitstreamPtr /*this->mOutput >> 1*/;
        //thisPtr = this;
        output_q = (Uint32*)outputBlockPtr /*this->mCoEffsBuffer*/;               // off 10 quantised coefficients
        endPtr = outptr - 1;

        do
        {
            ++endPtr;
        } while (*endPtr == 0xFE00u);  // 0xFE00 == END_OF_BLOCK, hence this loop moves past the EOB

        *output_q = (v1 << 10) + 2 * (*endPtr << 21 >> 22);
        dataPtr = endPtr + 1; // last use of endPtr


        if (*(Uint8 *)(dataPtr - 1) & 1)        // has video flag?
        {

            do
            {
                macroBlockWord1 = *dataPtr++;// bail if end
                if (macroBlockWord1 == 0xFE00)
                {
                    break;
                }
                Uint32* v21 = (macroBlockWord1 >> 10) + v6;
                v22 = (macroBlockWord1 >> 10) + counter;
                v23 = g_block_related_1_dword_42B0C8[v22];
                signed int v24 = output_q[v23] + (macroBlockWord1 << 22);
                SetHiWord(v25, GetHiWord(v24));
                counter = v22 + 1;
                SetLoWord(v25, (*(v21)* (v24 >> 22) + 4) >> 3);
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
                Uint32* v17 = v16 + v6;
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
                SetLoWord(outVal, (*(v17)* (v14 >> 22) + 4) >> 3);
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
                    output_q[g_block_related_unknown_dword_42B0C4[blockNumberQ++]] = 0;
                    if (blockNumberQ & 3)
                    {
                        output_q[g_block_related_unknown_dword_42B0C4[blockNumberQ++]] = 0;
                        if (blockNumberQ & 3)
                        {
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
                    // blockNumberQ++;
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


    void Masher::ParseVideoFrame()
    {
        if (mNumMacroblocksX <= 0 || mNumMacroblocksY <= 0)
        {
            return;
        }

        const int quantScale = decode_bitstream((Uint16*)mVideoFrameData.data(), mDecodedVideoFrameData.data());
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

                //ConvertYuvToRgbAndBlit(buf, xoff, yoff);

                yoff += 16;
            }
            xoff += 16;
        }

    }

    void Masher::ParseAudioFrame()
    {

    }

    bool Masher::Update()
    {
        if (mCurrentFrame < mFileHeader.mNumberOfFrames)
        {
            if (mbHasVideo && mbHasAudio)
            {
                // If there is video and audio then the first dword is
                // the size of the video data, and the audio data is
                // the remaining data after this.
                uint32_t videoDataSize = 0;
                mStream.ReadUInt32(videoDataSize);

                // Video data
                mVideoFrameData.resize(videoDataSize);
                mStream.ReadBytes(mVideoFrameData.data(), videoDataSize);

                // Calc size of audio data
                const uint32_t totalSize = mFrameSizes[mCurrentFrame];
                const uint32_t audioDataSize = totalSize - videoDataSize;

                // Audio data
                mAudioFrameData.resize(audioDataSize);
                mStream.ReadBytes(mAudioFrameData.data(), audioDataSize);
                ParseVideoFrame();
                ParseAudioFrame();
            }
            else if (mbHasAudio)
            {
                const uint32_t totalSize = mFrameSizes[mCurrentFrame];
                mAudioFrameData.resize(totalSize);
                mStream.ReadBytes(mAudioFrameData.data(), totalSize);
                ParseAudioFrame();
            }
            else if (mbHasVideo)
            {
                const uint32_t totalSize = mFrameSizes[mCurrentFrame];
                mVideoFrameData.resize(totalSize);
                mStream.ReadBytes(mVideoFrameData.data(), totalSize);
                ParseVideoFrame();
            }
            mCurrentFrame++;
            return true;
        }
        return false;
    }
}
