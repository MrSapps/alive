#include "audioconverter.hpp"
#include "resourcemapper.hpp"

template void AudioConverter::Convert<OggEncoder>(ISound& sound, const char* outputName);
template void AudioConverter::Convert<WavEncoder>(ISound& sound, const char* outputName);

template<class EncoderAlgorithm>
void AudioConverter::Convert(ISound& sound, const char* outputName)
{
    TRACE_ENTRYEXIT;

    EncoderAlgorithm encoder(outputName);

    for (;;)
    {
        f32 buffer[1024] = {};

        sound.Update();

        sound.Play(buffer, 1024);
        const bool endOfAudio = sound.AtEnd();
        u32 numSamplesToUse = 1024;
        if (endOfAudio)
        {
            // Trim down the buffer so the trailing silence is chopped off
            for (u32 i = 1024; i-- > 0; )
            {
                if (buffer[i] != 0.0f)
                {
                    break;
                }
                numSamplesToUse--;
            }
        }

        encoder.Consume(buffer, numSamplesToUse * sizeof(f32));

        if (endOfAudio)
        {
            break;
        }
    }

    encoder.Finish();
}

void WavHeader::Write(Oddlib::IStream& stream)
{
    stream.Write(mRiff);
    stream.Write(mFileSize);
    stream.Write(mWAVE);
    stream.Write(mFmt);
    stream.Write(mWaveChunkSize);
    mWaveChunk.Write(stream);
    stream.Write(mDataDescriptionHeader);
    stream.Write(mDataSize);
}

void WavHeader::FixHeaderSizes(Oddlib::IStream& stream)
{
    const size_t fileSize = stream.Pos();

    // Re-calculate mFileSize and mDataSize
    mFileSize = static_cast<u32>(fileSize - sizeof(mRiff) - sizeof(mFileSize));
    mDataSize = static_cast<u32>(fileSize - sizeof(WavHeader));

    // Seek to mFileSize and re-write it
    stream.Seek(sizeof(mRiff));
    stream.Write(mFileSize);

    // Seek to mDataSize and re-write it
    stream.Seek(sizeof(WavHeader) - sizeof(u32));
    stream.Write(mDataSize);
}

void WavHeader::WaveChunk::Write(Oddlib::IStream& stream)
{
    stream.Write(mWaveTypeFormat);
    stream.Write(mNumberOfChannels);
    stream.Write(mNumberOfSamplesPerSecond);
    stream.Write(mBytesPerSecond);
    stream.Write(mBlockAlignment);
    stream.Write(mBitsPerSample);
}

WavEncoder::WavEncoder(const char* outputName) : mStream(outputName, Oddlib::IStream::ReadMode::ReadWrite)
{
    mHeader.mWaveChunk.mBitsPerSample = 32;
    mHeader.mWaveChunk.mNumberOfChannels = 2;
    mHeader.mWaveChunk.mBlockAlignment = (mHeader.mWaveChunk.mNumberOfChannels * mHeader.mWaveChunk.mBitsPerSample) / 8;
    mHeader.mWaveChunk.mNumberOfSamplesPerSecond = 44100L;
    mHeader.mWaveChunk.mBytesPerSecond = mHeader.mWaveChunk.mNumberOfSamplesPerSecond * mHeader.mWaveChunk.mBlockAlignment;

    // Gets fixed up later via FixHeaderSizes
    mHeader.mFileSize = 0;
    mHeader.mDataSize = 0;

    mHeader.Write(mStream);
}

void WavEncoder::Consume(float* readbuffer, long bufferSizeInBytes)
{
    const u32 kNumFloats = bufferSizeInBytes / sizeof(float);
    const u32 kFloatsPerChannel = kNumFloats / 2;
    u32 pos = 0;
    for (u32 i = 0; i < kFloatsPerChannel; i++)
    {
        const float left = readbuffer[pos++];
        const float right = readbuffer[pos++];

        mStream.Write(left);
        mStream.Write(right);
    }
}

void WavEncoder::Finish()
{
    mHeader.FixHeaderSizes(mStream);
}

OggEncoder::OggEncoder(const char* outputName)
{
    output = fopen(outputName, "wb");
    InitEncoder();
}

OggEncoder::~OggEncoder()
{
    /* clean up and exit.  vorbis_info_clear() must be called last */
    ogg_stream_clear(&os);
    vorbis_block_clear(&vb);
    vorbis_dsp_clear(&vd);
    vorbis_comment_clear(&vc);
    vorbis_info_clear(&vi);
    fclose(output);
}

void OggEncoder::Consume(float* readbuffer, long bufferSizeInBytes)
{
    if (bufferSizeInBytes == 0)
    {
        // Mark as the last frame
        vorbis_analysis_wrote(&vd, 0);
    }
    else
    {
        /* data to encode */
        int numFloats = bufferSizeInBytes / sizeof(float);
        const auto numChannels = 2;
        const auto floatsPerChannel = numFloats / numChannels;

        /* expose the buffer to submit data */
        float** buffer = vorbis_analysis_buffer(&vd, floatsPerChannel); // 32bit float buffer

        int pos = 0;
        for (auto i = 0; i < floatsPerChannel; i++)
        {
            buffer[0][i] = readbuffer[pos++];
            buffer[1][i] = readbuffer[pos++];
        }

        /* tell the library how much we actually submitted */
        vorbis_analysis_wrote(&vd, floatsPerChannel);
    }

    /* vorbis does some data preanalysis, then divvies up blocks for
    more involved (potentially parallel) processing.  Get a single
    block for encoding now */
    while (vorbis_analysis_blockout(&vd, &vb) == 1)
    {
        /* analysis, assume we want to use bitrate management */
        vorbis_analysis(&vb, NULL);
        vorbis_bitrate_addblock(&vb);

        while (vorbis_bitrate_flushpacket(&vd, &op))
        {
            /* weld the packet into the bitstream */
            ogg_stream_packetin(&os, &op);

            /* write out pages (if any) */
            int result = ogg_stream_pageout(&os, &og);
            if (result == 0)
            {
                // Error or not enough data, ogg_stream_flush can force a new page
                break;
            }

            fwrite(og.header, 1, og.header_len, output);
            fwrite(og.body, 1, og.body_len, output);
        }
    }
}

void OggEncoder::InitEncoder()
{
    vorbis_info_init(&vi);

    /*int ret =*/ vorbis_encode_init_vbr(&vi, 2, 44100, 0.1f);

    // Appears in the file info to show which app created it
    vorbis_comment_init(&vc);
    vorbis_comment_add_tag(&vc, "ENCODER", "A.L.I.V.E");

    /* set up the analysis state and auxiliary encoding storage */
    vorbis_analysis_init(&vd, &vi);
    vorbis_block_init(&vd, &vb);

    /* set up our packet->stream encoder */
    /* pick a random serial number; that way we can more likely build
    chained streams just by concatenation */
    srand(static_cast<unsigned int>(time(NULL)));
    ogg_stream_init(&os, rand());

    /* Vorbis streams begin with three headers; the initial header (with
    most of the codec setup parameters) which is mandated by the Ogg
    bitstream spec.  The second header holds any comment fields.  The
    third header holds the bitstream codebook.  We merely need to
    make the headers, then pass them to libvorbis one at a time;
    libvorbis handles the additional Ogg bitstream constraints */
    ogg_packet header;
    ogg_packet header_comm;
    ogg_packet header_code;

    vorbis_analysis_headerout(&vd, &vc, &header, &header_comm, &header_code);
    ogg_stream_packetin(&os, &header); // automatically placed in its own page
    ogg_stream_packetin(&os, &header_comm);
    ogg_stream_packetin(&os, &header_code);

    /* This ensures the actual
    * audio data will start on a new page, as per spec
    */
    /*int result =*/ ogg_stream_flush(&os, &og);


    fwrite(og.header, 1, og.header_len, output);
    fwrite(og.body, 1, og.body_len, output);
}
