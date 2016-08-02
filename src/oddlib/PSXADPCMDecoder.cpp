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

void PSXADPCMDecoder::VagTest(Oddlib::IStream& stream)
{
    stream.Seek(0x40);

    double f[5][2] = { { 0.0, 0.0 },
    { 60.0 / 64.0, 0.0 },
    { 115.0 / 64.0, -52.0 / 64.0 },
    { 98.0 / 64.0, -55.0 / 64.0 },
    { 122.0 / 64.0, -60.0 / 64.0 } };

    double samples[28];
    double  s_2 = 0.0;
    double  s_1 = 0.0;

    std::vector<Uint8> output;

    for (;;)
    {
        Uint8 predict_nr = 0;
        stream.ReadUInt8(predict_nr);
        
        int shift_factor = predict_nr & 0xf;
        predict_nr >>= 4;


        Uint8 flags = 0;
        stream.ReadUInt8(flags);

        if (flags & 0x1) // todo bit 0 or 1?
        {
            std::ofstream rawData("Raw.dat", std::ios::binary);
            rawData.write((const char*)output.data(), output.size());
            return;
        }


        for (int i = 0; i < 28; i += 2)
        {
            Uint8 d = 0;
            stream.ReadUInt8(d);
            Uint32 s = (d & 0xf) << 12;
            if (s & 0x8000)
            {
                s |= 0xffff0000;
            }
            samples[i] = (double)(s >> shift_factor);
            s = (d & 0xf0) << 8;
            if (s & 0x8000)
            {
                s |= 0xffff0000;
            }
            samples[i + 1] = (double)(s >> shift_factor);
        }

        for (int i = 0; i < 28; i++)
        {
            samples[i] = samples[i] + s_1 * f[predict_nr][0] + s_2 * f[predict_nr][1];
            s_2 = s_1;
            s_1 = samples[i];
            short d = (short)(samples[i] + 0.5);

            output.push_back(d & 0xff);
            output.push_back(static_cast<unsigned char>(d >> 8));
        }
    }

    /*
   
    const int f0 = pos_adpcm_table[filter];
    const int f1 = neg_adpcm_table[filter];

    int older = 0;
    int old = 0;
    int nibble = 0;

    for (int d = 0; d < 28/2; d++)
    {
        const int t = Signed4bit((Uint8)((sampleData[d] >> (nibble * 4)) & 0xF));
        int s = (short)((t << shift) + ((old * f0 + older * f1 + 32) / 64));
        s = static_cast<Uint16>(MinMax(s, -32768, 32767));

        //out[dst] = static_cast<short>(s);
        //dst += 2;

        older = old;
        old = static_cast<short>(s);
    }
    */
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
