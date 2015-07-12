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


uint16_t PSXADPCMDecoder::DecodeFrameToPCM(int8_t *arg_decoded_stream,
    uint8_t *arg_adpcm_frame,
    bool arg_stereo)
{
    SoundFrame *sf = (SoundFrame *)arg_adpcm_frame;
    uint16_t count;
    uint16_t output_bytes = 0;
    int8_t unit_data, unit_filter, unit_range;
    int16_t decoded;
    int32_t tmp2, tmp3, tmp4, tmp5;

    for (uint8_t sound_group = 0; sound_group < 18; sound_group++)
    {
        count = 0;

        for (uint32_t sound_unit = 0; sound_unit < 8;
            sound_unit += arg_stereo + 1)
        {
            unit_range = GetRange(sf->sound_groups[sound_group], sound_unit);
            unit_filter = GetFilter(sf->sound_groups[sound_group], sound_unit);

            for (uint8_t sound_sample = 0; sound_sample < 28; sound_sample++)
            {
                // channel 1
                unit_data = GetSoundData(sf->sound_groups[sound_group],
                    sound_unit, sound_sample);
                tmp2 = (int32_t)(unit_data) << (12 - unit_range);
                tmp3 = (int32_t)tmp2 * 2;
                tmp4 = FixedMultiply(K0[unit_filter], t1);
                tmp5 = FixedMultiply(K1[unit_filter], t2);
                t2 = t1;
                t1 = tmp3 + tmp4 + tmp5;
                decoded = MaxMin(t1 / 2);

                arg_decoded_stream[output_bytes + count++] =
                    (int8_t)(decoded & 0x0000ffff);
                arg_decoded_stream[output_bytes + count++] =
                    (int8_t)(decoded >> 8);


                if (arg_stereo)
                {
                    // channel 2
                    unit_data = GetSoundData(sf->sound_groups[sound_group],
                        sound_unit + 1, sound_sample);
                    int8_t unit_range1 = GetRange(sf->sound_groups[sound_group],
                        sound_unit + 1);
                    tmp2 = (int32_t)(unit_data) << (12 - unit_range1);
                    tmp3 = (int32_t)tmp2 * 2;
                    int8_t unit_filter1 =
                        GetFilter(sf->sound_groups[sound_group],
                        sound_unit + 1);
                    tmp4 = FixedMultiply(K0[unit_filter1], t1_x);
                    tmp5 = FixedMultiply(K1[unit_filter1], t2_x);
                    t2_x = t1_x;
                    t1_x = tmp3 + tmp4 + tmp5;
                    decoded = MaxMin(t1_x / 2);

                    arg_decoded_stream[output_bytes + count++] =
                        (int8_t)(decoded & 0x0000ffff);
                    arg_decoded_stream[output_bytes + count++] =
                        (int8_t)(decoded >> 8);
                }

            }
        }

        output_bytes += count;
    }

    return output_bytes;
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

