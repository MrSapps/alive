#pragma once

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdint.h>
#include <vector>
#include "SDL_stdinc.h"
#include "oddlib/stream.hpp"


class PSXADPCMDecoder
{
public:
    PSXADPCMDecoder() = default;
    void DecodeFrameToPCM(std::vector<Sint16>& out, uint8_t *arg_adpcm_frame);
    void DecodeVagStream(Oddlib::IStream& s, std::vector<Uint8>& out);

public:
#pragma pack(push)
#pragma pack(1)
    struct SoundFrame
    {
        struct SoundGroup
        {
            uint8_t sound_parameters[16];
            uint8_t audio_sample_bytes[112];
        };

        SoundGroup sound_groups[18];
        uint8_t unused_for_adpcm[20];
        uint8_t edc[4];
    };

#pragma pack(pop)
};
