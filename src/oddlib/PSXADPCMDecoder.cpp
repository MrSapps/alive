/*
 * PSX ADPCM Audio Decoder
 *
 * Copyright (C) (2007) by G. <gennadiy.brich@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */



#include "oddlib/PSXADPCMDecoder.h"

#include <algorithm>



const uint32_t PSXADPCMDecoder::K0[4] =
{ 0x00000000, 0x0000F000, 0x0001CC00, 0x00018800 };


const uint32_t PSXADPCMDecoder::K1[4] =
{ 0x00000000, 0x00000000, 0xFFFF3000, 0xFFFF2400 };



PSXADPCMDecoder::PSXADPCMDecoder() :
t1(0), t2(0), t1_x(0), t2_x(0)
{
    ;
}


PSXADPCMDecoder::~PSXADPCMDecoder()
{
    ;
}

#define AUDIO_DATA_CHUNK_SIZE   2304
#define AUDIO_DATA_SAMPLE_COUNT 4032

static const int s_xaTable[5][2] = {
        { 0, 0 },
        { 60, 0 },
        { 115, -52 },
        { 98, -55 },
        { 122, -60 }
};
struct ADPCMStatus {
    int16_t sample[2];
} _adpcmStatus[2] = {};
template <typename T>

T CLIP(const T& n, const T& lower, const T& upper) {
    return std::max(lower, std::min(n, upper));
}

uint16_t PSXADPCMDecoder::DecodeFrameToPCM(int8_t *arg_decoded_stream, uint8_t *arg_adpcm_frame, bool arg_stereo)
{
   // sector->seek(24);
    arg_adpcm_frame += 8;

    // This XA audio is different (yet similar) from normal XA audio! Watch out!
    // TODO: It's probably similar enough to normal XA that we can merge it somehow...
    // TODO: RTZ PSX needs the same audio code in a regular AudioStream class. Probably
    // will do something similar to QuickTime and creating a base class 'ISOMode2Parser'
    // or something similar.
    uint8_t *buf = arg_adpcm_frame;

    int channels = true ? 2 : 1;
    int16_t *dst = (int16_t*)arg_decoded_stream;
    int16_t *leftChannel = dst;
    int16_t *rightChannel = dst + 1;

    for (uint8_t *src = buf; src < buf + AUDIO_DATA_CHUNK_SIZE; src += 128)
    {
        for (int i = 0; i < 4; i++)
        {
            int shift = 12 - (src[4 + i * 2] & 0xf);
            int filter = src[4 + i * 2] >> 4;
            int f0 = s_xaTable[filter][0];
            int f1 = s_xaTable[filter][1];
            int16_t s_1 = _adpcmStatus[0].sample[0];
            int16_t s_2 = _adpcmStatus[0].sample[1];

            for (int j = 0; j < 28; j++) 
            {
                uint8_t d = src[16 + i + j * 4];
                int t = (int8_t)(d << 4) >> 4;
                int s = (t << shift) + ((s_1 * f0 + s_2 * f1 + 32) >> 6);
                s_2 = s_1;
                s_1 = CLIP<int>(s, -32768, 32767);

                *leftChannel = s_1;
                leftChannel += channels;
            }

            if (channels == 2) 
            {
                _adpcmStatus[0].sample[0] = s_1;
                _adpcmStatus[0].sample[1] = s_2;
                s_1 = _adpcmStatus[1].sample[0];
                s_2 = _adpcmStatus[1].sample[1];
            }

            shift = 12 - (src[5 + i * 2] & 0xf);
            filter = src[5 + i * 2] >> 4;
            f0 = s_xaTable[filter][0];
            f1 = s_xaTable[filter][1];

            for (int j = 0; j < 28; j++)
            {
                uint8_t d = src[16 + i + j * 4];
                int t = (int8_t)d >> 4;
                int s = (t << shift) + ((s_1 * f0 + s_2 * f1 + 32) >> 6);
                s_2 = s_1;
                s_1 = CLIP<int>(s, -32768, 32767);


                if (channels == 2)
                {
                    *rightChannel = s_1;
                    rightChannel += 2;
                }
                else 
                {
                    *leftChannel++ = s_1;
                }
            }

            if (channels == 2)
            {
                _adpcmStatus[1].sample[0] = s_1;
                _adpcmStatus[1].sample[1] = s_2;
            }
            else 
            {
                _adpcmStatus[0].sample[0] = s_1;
                _adpcmStatus[0].sample[1] = s_2;
            }
        }
    }

   // _audStream->queueBuffer((byte *)dst, AUDIO_DATA_SAMPLE_COUNT * 2, DisposeAfterUse::YES, flags);

    return AUDIO_DATA_SAMPLE_COUNT * 2;
}


inline int8_t PSXADPCMDecoder::GetSoundData(const SoundFrame::SoundGroup &arg_group,
    uint8_t arg_unit,
    uint8_t arg_sample)
{
    int8_t result = (*((uint8_t *)arg_group.audio_sample_bytes + (arg_unit / 2)
        + (arg_sample * 4)) >> ((arg_unit % 2) * 4)) & 0x0F;

    if (result > 7)
        result -= 16;

    return result;
}


inline int8_t PSXADPCMDecoder::GetFilter(const SoundFrame::SoundGroup &arg_group,
    uint8_t arg_unit)
{
    return (arg_group.sound_parameters[arg_unit + 4] >> 4) & 0x03;
}


inline int8_t PSXADPCMDecoder::GetRange(const SoundFrame::SoundGroup &arg_group,
    uint8_t arg_unit)
{
    return arg_group.sound_parameters[arg_unit + 4] & 0x0f;
}


int32_t PSXADPCMDecoder::FixedMultiply(int32_t arg_a, int32_t arg_b)
{
    int32_t high_a, low_a, high_b, low_b;
    int32_t hahb, halb, lahb;
    uint32_t lalb;
    int32_t result;

    high_a = arg_a >> 16;
    low_a = arg_a & 0x0000FFFF;
    high_b = arg_b >> 16;
    low_b = arg_b & 0x0000FFFF;

    hahb = (high_a * high_b) << 16;
    halb = high_a * low_b;
    lahb = low_a * high_b;
    lalb = (uint32_t)(low_a * low_b) >> 16;

    result = hahb + halb + lahb + lalb;

    return result;
}


inline int32_t PSXADPCMDecoder::MaxMin(int32_t arg_number)
{
    int32_t temp = (arg_number > 32767) ? 32767 : arg_number;

    return (temp < -32768) ? -32768 : temp;
}

