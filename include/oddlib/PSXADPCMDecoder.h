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



#ifndef PSX_ADPCM_DECODER_H__
#define PSX_ADPCM_DECODER_H__



#include <stdint.h>



class PSXADPCMDecoder
{
public:
    PSXADPCMDecoder();
    ~PSXADPCMDecoder();
    uint16_t DecodeFrameToPCM(int8_t *arg_decoded_stream,
        uint8_t *arg_adpcm_frame,
        bool arg_stereo);


private:
#pragma pack(push)
#pragma pack(1)
    struct SoundFrame
    {
        struct SoundGroup
        {
            uint8_t sound_parameters[16];
            uint8_t audio_sample_bytes[112];
        } sound_groups[18];

        uint8_t unused_for_adpcm[20];
        uint8_t edc[4];
    };
#pragma pack(pop)
    static const uint32_t K0[4]; // TODO: Was signed, not sure which is correct though
    static const uint32_t K1[4];

    int32_t t1, t2;
    int32_t t1_x, t2_x;

    int8_t GetSoundData(const SoundFrame::SoundGroup &arg_group,
        uint8_t arg_unit,
        uint8_t arg_sample);
    inline int8_t GetFilter(const SoundFrame::SoundGroup &arg_group,
        uint8_t arg_unit);
    inline int8_t GetRange(const SoundFrame::SoundGroup &arg_group,
        uint8_t arg_unit);
    inline int32_t FixedMultiply(int32_t a, int32_t b);
    inline int32_t MaxMin(int32_t arg_number);
};



#endif //PSX_ADPCM_DECODER_H__

