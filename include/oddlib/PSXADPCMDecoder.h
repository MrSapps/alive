#pragma once

#include <stdint.h>
#include <vector>
#include "SDL_stdinc.h"

class PSXADPCMDecoder
{
public:
    PSXADPCMDecoder() = default;
    void DecodeFrameToPCM(std::vector<Sint16>& out,
        uint8_t *arg_adpcm_frame,
        bool arg_stereo);


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
