#pragma once

#include "oddlib/stream.hpp"
#include "oddlib/lvlarchive.hpp"
#include <vorbis/vorbisenc.h>

class ISound;

class OggDecoder // TODO
{
public:

};

class AudioConverter
{
public:
    AudioConverter() = delete;

    template<class EncoderAlgorithm>
    static void Convert(ISound& sound, const char* outputName);
};

class WavHeader
{
public:
    void Write(Oddlib::IStream& stream);
    void FixHeaderSizes(Oddlib::IStream& stream);


    enum eWaveFormats
    {
        eIEEEFloat = 3,
    };

    struct WaveChunk
    {
        u16 mWaveTypeFormat = eIEEEFloat; // 1 = PCM
        u16 mNumberOfChannels;
        u32 mNumberOfSamplesPerSecond;
        u32 mBytesPerSecond;
        u16 mBlockAlignment;
        u16 mBitsPerSample;

        void Write(Oddlib::IStream& stream);
    };
    static_assert(sizeof(WaveChunk) == 16, "WaveChunk must be 16 bytes");
    
    struct Header
    {
        u32 mRiff = Oddlib::MakeType("RIFF");
        u32 mFileSize = 0;
        u32 mWAVE = Oddlib::MakeType("WAVE");
        u32 mFmt = Oddlib::MakeType("fmt ");
        u32 mWaveChunkSize = sizeof(WaveChunk);
        WaveChunk mWaveChunk;
        u32 mDataDescriptionHeader = Oddlib::MakeType("data");
        u32 mDataSize = 0;
    };
    Header mData;
    //static_assert(sizeof(WaveChunk) == 44, "Header must be 44 bytes");

};

class WavEncoder
{
public:
    WavEncoder(const char* outputName);
    void Consume(float* readbuffer, long bufferSizeInBytes);
    void Finish();
private:
    WavHeader mHeader;
    Oddlib::FileStream mStream;
};

class OggEncoder
{
public:
    explicit OggEncoder(const char* outputName);
    ~OggEncoder();
    void Consume(float* readbuffer, long bufferSizeInBytes);
    void Finish() { }
private:
    void InitEncoder();

    FILE*            output = nullptr;
    ogg_stream_state os; // take physical pages, weld into a logical stream of packets
    ogg_page         og; // one Ogg bitstream page.  Vorbis packets are inside
    ogg_packet       op; // one raw packet of data for decode
    vorbis_info      vi; // struct that stores all the static vorbis bitstream settings
    vorbis_comment   vc; // struct that stores all the user comments
    vorbis_dsp_state vd; // central working state for the packet->PCM decoder
    vorbis_block     vb; // local working space for packet->PCM decode
};
