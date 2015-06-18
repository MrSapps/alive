#pragma once

#include "stream.hpp"

namespace Oddlib
{
    // Implements Digital Dialect Video (DDV) version 1. This is designed to work only
    // with existing DDV video files, creating new videos with the "Masher" tool may
    // result in a file that this code can't handle.
    class Masher
    {
    public:
        Masher(const Masher&) = delete;
        Masher& operator = (const Masher&) = delete;
        explicit Masher(std::string fileName) :
            mStream(fileName)
        {
            Read();
        }

        bool Update();

    private:
        void Read();

        struct DDVHeader
        {
            uint32_t mDdvTag;
            uint32_t mDdvVersion;
            uint32_t mContains;
            uint32_t mFrameRate;
            uint32_t mNumberOfFrames;
        };

        struct VideoHeader
        {
            uint32_t mUnknown; // Probably a reserved field, it has no effect and isn't read by masher lib
            uint32_t mWidth;
            uint32_t mHeight;
            uint32_t mMaxVideoFrameSize;
            uint32_t mMaxAudioFrameSize; // size of a buffer that contains shorts, i.e read mSingleAudioFrameSize * 2
            uint32_t mKeyFrameRate;
        };

        struct AudioHeader
        {
            uint32_t mAudioFormat;
            uint32_t mSampleRate;
            uint32_t mMaxAudioFrameSize;
            uint32_t mSingleAudioFrameSize; // sampleRate / fps, i.e 44100/15=2940
            uint32_t mNumberOfFramesInterleave;
        };

        Stream mStream;

        DDVHeader mFileHeader = {};
        VideoHeader mVideoHeader = {};
        AudioHeader mAudioHeader = {};

        bool mbHasAudio;
        bool mbHasVideo;

        uint32_t mNumMacroblocksX = 0;
        uint32_t mNumMacroblocksY = 0;

        std::vector<uint32_t> mAudioFrameSizes;
        std::vector<uint32_t> mVideoFrameSizes;

        uint32_t mCurrentFrame = 0;
    };
}