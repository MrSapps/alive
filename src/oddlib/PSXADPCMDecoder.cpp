#include "oddlib/PSXADPCMDecoder.h"
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

static void DecodeBlock(
    std::vector<Sint16>& out,
    const Uint8(&samples)[112],
    const Uint8(&parameters)[16],
    Uint8 block,
    Uint8 nibble,
    short& dst,
    short& old,
    short& older)
{
    // 4 blocks for each nibble, so 8 blocks total
    const int shift = (Uint8)(12 - (parameters[4 + block * 2 + nibble] & 0xF));
    const int filter = (Sint8)((parameters[4 + block * 2 + nibble] & 0x30) >> 4);

    const int f0 = pos_adpcm_table[filter];
    const int f1 = neg_adpcm_table[filter];

    for (int d = 0; d < 28; d++)
    {
        const int t = Signed4bit((Uint8)((samples[block + d * 4] >> (nibble * 4)) & 0xF));
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
    static short oldLeft = 0;
    static short olderLeft = 0;
    
    short dstRight = 1;
    static short oldRight = 0;
    static short olderRight = 0;

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

void PSXADPCMDecoder::DecodeFrameToPCM(std::vector<Sint16>& out, uint8_t *arg_adpcm_frame, bool /*arg_stereo*/)
{
    const PSXADPCMDecoder::SoundFrame* sf = reinterpret_cast<const PSXADPCMDecoder::SoundFrame*>(arg_adpcm_frame);
    Decode(*sf, out);
}
