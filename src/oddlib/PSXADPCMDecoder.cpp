#include "oddlib/PSXADPCMDecoder.h"
#include "logger.hpp"
#include <algorithm>
#include "types.hpp"

// Pos / neg Tables
const short pos_adpcm_table[5] = { 0, +60, +115, +98, +122 };
const short neg_adpcm_table[5] = { 0, 0, -52, -55, -60 };

static s8 Signed4bit(u8 number)
{
    if ((number & 0x8) == 0x8)
    {
        return (number & 0x7) - 8;
    }
    else
    {
        return number;
    }
}

static int MinMax(int number, int min, int max)
{
    if (number < min)
    {
        return min;
    }

    if (number > max)
    {
        return max;
    }

    return number;
}

static int SignExtend(int s)
{
    if (s & 0x8000)
    {
        s |= 0xffff0000;
    }
    return s;
}

static void DecodeNibble(bool firstNibble, int shift, int filter, int d, f64& old, f64& older, FILE* pcm, std::vector<u8>* out = nullptr)
{
    const f64 f0 = static_cast<f64>(pos_adpcm_table[filter]) / 64.0f;
    const f64 f1 = static_cast<f64>(neg_adpcm_table[filter]) / 64.0f;

    int s = firstNibble ? (d & 0x0f) << 12 : (d & 0xf0) << 8;
    s = SignExtend(s);
    
    const f64 sample = static_cast<f64>(s >> shift) + old * f0 + older * f1;

    older = old;
    old = sample;

    const int x = (int)(sample + 0.5);
    if (pcm)
    {
        fputc(x & 0xff, pcm);
        fputc(x >> 8, pcm);
    }

    if (out)
    {
        out->push_back(x & 0xff);
        out->push_back(static_cast<u8>(x >> 8));
    }
}

void PSXADPCMDecoder::DecodeVagStream(Oddlib::IStream& s, std::vector<u8>& out)
{
    f64 old = 0.0;
    f64 older = 0.0;

    for (;;)
    {
        // Filter and shift nibbles
        u8 filter = 0;
        s.ReadUInt8(filter);
        const int shift = filter & 0xf;
        filter >>= 4;

        // Flags byte
        u8 flags = 0;
        s.ReadUInt8(flags);
        if (flags & 1) // EOF flag, checking for == 7 is wrong
        {
            break;
        }
        else if ((flags & 4) > 0)
        {
            // flags & 2 == loop?
            // flags & 4 == sampler loop?

            // TODO: Handle loop flag
            //LOG_INFO("TODO: Sampler loop");
        }

        // 14 bytes of data
        for (int i = 0; i < 28 / 2; i++)
        {
            u8 tmp = 0;
            s.ReadUInt8(tmp);
            DecodeNibble(true, shift, filter, tmp, old, older, nullptr, &out);
            DecodeNibble(false, shift, filter, tmp, old, older, nullptr, &out);
        }
    }

}

static void DecodeBlock(
    std::vector<s16>& out,
    const u8(&samples)[112],
    const u8(&parameters)[16],
    u8 block,
    u8 nibble,
    short& dst,
    f64& old,
    f64& older)
{
    // 4 blocks for each nibble, so 8 blocks total
    const int shift = (u8)(12 - (parameters[4 + block * 2 + nibble] & 0xF));
    const int filter = (s8)((parameters[4 + block * 2 + nibble] & 0x30) >> 4);

    const int f0 = pos_adpcm_table[filter];
    const int f1 = neg_adpcm_table[filter];

    for (int d = 0; d < 28; d++)
    {
        const u8 value = samples[block + d * 4];
        const int t = Signed4bit((u8)((value >> (nibble * 4)) & 0xF));
        int s = (short)((t << shift) + ((old * f0 + older * f1 + 32) / 64));
        s = static_cast<u16>(MinMax(s, -32768, 32767));
       

        out[dst] = static_cast<short>(s);
        dst += 2;

        older = old;
        old = static_cast<short>(s);
    }
}

static void Decode(const PSXADPCMDecoder::SoundFrame& sf, std::vector<s16>& out)
{
    short dstLeft = 0;
    static f64 oldLeft = 0;
    static f64 olderLeft = 0;
    
    short dstRight = 1;
    static f64 oldRight = 0;
    static f64 olderRight = 0;

    for (int i = 0; i<18; i++)
    {
        const PSXADPCMDecoder::SoundFrame::SoundGroup& sg = sf.sound_groups[i];
        for (u8 b = 0; b < 4; b++)
        {
            DecodeBlock(out, sg.audio_sample_bytes, sg.sound_parameters, b, 1, dstLeft, oldLeft, olderLeft);
            DecodeBlock(out, sg.audio_sample_bytes, sg.sound_parameters, b, 0, dstRight, oldRight, olderRight);
        }
    }
}

void PSXADPCMDecoder::DecodeFrameToPCM(std::vector<s16>& out, uint8_t* arg_adpcm_frame)
{
    const PSXADPCMDecoder::SoundFrame* sf = reinterpret_cast<const PSXADPCMDecoder::SoundFrame*>(arg_adpcm_frame);
    Decode(*sf, out);
}
