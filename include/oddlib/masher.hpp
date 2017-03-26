#pragma once

#include "types.hpp"
#include "stream.hpp"
#include "oddlib/exceptions.hpp"


namespace Oddlib
{
    class AudioDecompressor
    {
    public:

        s32 mUsedBits = 0;
        u32 mWorkBits = 0;
        s32 mAudioFrameSizeBytes = 0;
        u16* mAudioFrameDataPtr = nullptr;

        static u8 gSndTbl_byte_62EEB0[256];

        AudioDecompressor();
        static s32 GetSoundTableValue(s16 tblIndex);
        s16 sub_408F50(s16 a1);
        s32 ReadNextAudioWord(s32 value);
        s32 SndRelated_sub_409650();
        s16 NextSoundBits(u16 numBits);
        bool SampleMatches(s16& sample, s16 bits);
        void decode_16bit_audio_frame(u16* outPtr, s32 numSamplesPerFrame, bool isLast);
        u16* SetupAudioDecodePtrs(u16 *rawFrameBuffer);
        s32 SetAudioFrameSizeBytesAndBits(s32 audioFrameSizeBytes);
        static void init_Snd_tbl();
    };

    class InvalidDdv : public Exception
    {
    public:
        explicit InvalidDdv(const char* msg) : Exception(msg) { }
    };

    // Implements Digital Dialect Video (DDV) version 1. This is designed to work only
    // with existing DDV video files, creating new videos with the "Masher" tool may
    // result in a file that this code can't handle.
    class Masher
    {
    public:
        Masher() = default;
        Masher(const Masher&) = delete;
        Masher& operator = (const Masher&) = delete;

        explicit Masher(std::unique_ptr<Oddlib::IStream> stream) : mStream(std::move(stream))
        {
            Read();
        }

        bool Update(u32* pixelBuffer, u8* audioBuffer);

        u32 Width() const  { return mVideoHeader.mWidth;  }
        u32 Height() const { return mVideoHeader.mHeight; }
        bool HasVideo() const { return mbHasVideo; }
        bool HasAudio() const { return mbHasAudio; }
        u32 SingleAudioFrameSizeSamples() const { return mAudioHeader.mSingleAudioFrameSize; }
        u32 AudioSampleRate() const { return mAudioHeader.mSampleRate; }
        u32 FrameNumber() const { return mCurrentFrame; }
        u32 FrameRate() const { return mFileHeader.mFrameRate; }
        u32 NumberOfFrames() const { return mFileHeader.mNumberOfFrames; }
    protected:
        void decode_audio_frame(u16 *rawFrameBuffer, u16 *outPtr, signed int numSamplesPerFrame);
    private:
        void Read();
        void ParseVideoFrame(u32* pixelBuffer);

        void ParseAudioFrame(u8* audioBuffer);
       
        void do_decode_audio_frame(u8* audioBuffer);

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
            uint32_t mMaxAudioFrameSize;
            uint32_t mMaxVideoFrameSize;
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

        std::unique_ptr<Oddlib::IStream> mStream;

        DDVHeader mFileHeader = {};
        VideoHeader mVideoHeader = {};
        AudioHeader mAudioHeader = {};

        bool mbHasAudio;
        bool mbHasVideo;

        uint32_t mNumMacroblocksX = 0;
        uint32_t mNumMacroblocksY = 0;

        std::vector<uint32_t> mAudioFrameSizes;
        std::vector<uint32_t> mFrameSizes;

        uint32_t mCurrentFrame = 0;

        std::vector<uint8_t> mVideoFrameData;
        std::vector<uint8_t> mAudioFrameData;

        std::vector<u16> mMacroBlockBuffer;

    protected:
        std::vector<u16> mDecodedVideoFrameData;
    };
}
