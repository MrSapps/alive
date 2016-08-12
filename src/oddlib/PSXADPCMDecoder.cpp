#include "oddlib/PSXADPCMDecoder.h"
#include "logger.hpp"
#include <algorithm>

// Pos / neg Tables
const short pos_adpcm_table[5] = { 0, +60, +115, +98, +122 };
const short neg_adpcm_table[5] = { 0, 0, -52, -55, -60 };

static Sint8 Signed4bit(Uint8 number)
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


static void DecodeNibble(bool firstNibble, int shift, int filter, int d, double& old, double& older, FILE* pcm, std::vector<Uint8>* out = nullptr)
{
    const double f0 = static_cast<double>(pos_adpcm_table[filter]) / 64.0f;
    const double f1 = static_cast<double>(neg_adpcm_table[filter]) / 64.0f;

    int s = firstNibble ? (d & 0x0f) << 12 : (d & 0xf0) << 8;
    s = SignExtend(s);
    
    const double sample = static_cast<double>(s >> shift) + old * f0 + older * f1;

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
        out->push_back(static_cast<Uint8>(x >> 8));
    }
}

void PSXADPCMDecoder::DecodeVagStream(Oddlib::IStream& s, std::vector<Uint8>& out)
{
    double old = 0.0;
    double older = 0.0;

    for (;;)
    {
        // Filter and shift nibbles
        Uint8 filter = 0;
        s.ReadUInt8(filter);
        const int shift = filter & 0xf;
        filter >>= 4;

        // Flags byte
        Uint8 flags = 0;
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
            Uint8 tmp = 0;
            s.ReadUInt8(tmp);
            DecodeNibble(true, shift, filter, tmp, old, older, nullptr, &out);
            DecodeNibble(false, shift, filter, tmp, old, older, nullptr, &out);
        }
    }

}

static void DecodeBlock(
    std::vector<Sint16>& out,
    const Uint8(&samples)[112],
    const Uint8(&parameters)[16],
    Uint8 block,
    Uint8 nibble,
    short& dst,
    double& old,
    double& older)
{
    // 4 blocks for each nibble, so 8 blocks total
    const int shift = (Uint8)(12 - (parameters[4 + block * 2 + nibble] & 0xF));
    const int filter = (Sint8)((parameters[4 + block * 2 + nibble] & 0x30) >> 4);

    const int f0 = pos_adpcm_table[filter];
    const int f1 = neg_adpcm_table[filter];

    for (int d = 0; d < 28; d++)
    {
        const Uint8 value = samples[block + d * 4];
        const int t = Signed4bit((Uint8)((value >> (nibble * 4)) & 0xF));
        int s = (short)((t << shift) + ((old * f0 + older * f1 + 32) / 64));
        s = static_cast<Uint16>(MinMax(s, -32768, 32767));
       

        out[dst] = static_cast<short>(s);
        dst += 2;

        older = old;
        old = static_cast<short>(s);
    }
}

static void Decode(const PSXADPCMDecoder::SoundFrame& sf, std::vector<Sint16>& out)
{
    short dstLeft = 0;
    static double oldLeft = 0;
    static double olderLeft = 0;
    
    short dstRight = 1;
    static double oldRight = 0;
    static double olderRight = 0;

    for (int i = 0; i<18; i++)
    {
        const PSXADPCMDecoder::SoundFrame::SoundGroup& sg = sf.sound_groups[i];
        for (Uint8 b = 0; b < 4; b++)
        {
            DecodeBlock(out, sg.audio_sample_bytes, sg.sound_parameters, b, 1, dstLeft, oldLeft, olderLeft);
            DecodeBlock(out, sg.audio_sample_bytes, sg.sound_parameters, b, 0, dstRight, oldRight, olderRight);
        }
    }
}

void PSXADPCMDecoder::DecodeFrameToPCM(std::vector<Sint16>& out, uint8_t* arg_adpcm_frame)
{
    const PSXADPCMDecoder::SoundFrame* sf = reinterpret_cast<const PSXADPCMDecoder::SoundFrame*>(arg_adpcm_frame);
    Decode(*sf, out);
}
