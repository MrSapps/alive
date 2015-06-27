#include "oddlib/masher.hpp"
#include "oddlib/exceptions.hpp"
#include "oddlib/lvlarchive.hpp"
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
            mStream.ReadUInt32(mVideoHeader.mMaxVideoFrameSize);
            mStream.ReadUInt32(mVideoHeader.mMaxAudioFrameSize);
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


        mVideoFrameData.resize(mVideoHeader.mMaxVideoFrameSize);
        mAudioFrameData.resize(mVideoHeader.mMaxAudioFrameSize);
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
            }
            else if (mbHasAudio)
            {
                const uint32_t totalSize = mFrameSizes[mCurrentFrame];
                mAudioFrameData.resize(totalSize);
                mStream.ReadBytes(mAudioFrameData.data(), totalSize);
            }
            else if (mbHasVideo)
            {
                const uint32_t totalSize = mFrameSizes[mCurrentFrame];
                mVideoFrameData.resize(totalSize);
                mStream.ReadBytes(mVideoFrameData.data(), totalSize);
            }
            mCurrentFrame++;
            return false;
        }
        return true;
    }
}
