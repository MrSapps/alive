#include "oddlib/masher.hpp"
#include "oddlib/exceptions.hpp"
#include "oddlib/lvlarchive.hpp"
#include "oddlib/masher_tables.hpp"
#include "logger.hpp"
#include <assert.h>

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


        mDecodedVideoFrameData.resize(mVideoHeader.mMaxVideoFrameSize);

        // TODO: Need work buffer for audio
        mAudioFrameData.resize(mVideoHeader.mMaxAudioFrameSize);
    }

    static void SetLoWord(Uint32& v, Uint16 lo)
    {
        Uint16 hiWord = v & 0xFFFF;
        v = (lo & 0xFFFF) | (hiWord << 16);
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

    void Masher::ParseVideoFrame()
    {
        const int quantScale = decode_bitstream((Uint16*)mVideoFrameData.data(), mDecodedVideoFrameData.data());

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
