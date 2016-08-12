#include "oddlib/audio/vab.hpp"
#include "oddlib/PSXADPCMDecoder.h"
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <map>
#include <stdlib.h>
#include <string.h>

void Vab::ReadVb(Oddlib::IStream& s, bool isPsx, bool useSoundsDat, Oddlib::IStream* soundsDatStream)
{
    mSamples.reserve(mHeader.iNumVags);

    if (isPsx)
    {
        // TODO: Some offsets might be duplicated
        for (u32& vag : mVagOffsets)
        {
            s.Seek(vag);
            PSXADPCMDecoder d;

            // TODO: Calculate expected size to reduce mem allocs - based on next vag offset/EOF
            std::vector<u8> data;
            data.reserve(8000); // Guess to help debug perf
            d.DecodeVagStream(s, data);
            mSamples.emplace_back(SampleData{ data });
        }
    }
    else
    {
        if (useSoundsDat)
        {
            for (auto i = 0; i < mHeader.iNumVags; i++)
            {
                iOffs.emplace_back(AEVh(s));
            }

            // Read sample buffers out of sounds.dat stream
            for (const AEVh& vhRec : iOffs)
            {
                soundsDatStream->Seek(vhRec.iFileOffset);
                std::vector<u8> data(vhRec.iLengthOrDuration);
                soundsDatStream->ReadBytes(data.data(), data.size());
                mSamples.emplace_back(SampleData{ data });
            }
        }
        else
        {
            for (auto i = 0; i < mHeader.iNumVags; ++i)
            {
                u32 size = 0;
                s.ReadUInt32(size);

                // Not actually used for anything
                u32 sampleRate = 0;
                s.ReadUInt32(sampleRate);

                if (size > 0)
                {
                    std::vector<u8> data(size);
                    s.ReadBytes(data.data(), data.size());
                    mSamples.emplace_back(SampleData{ data });
                }
                else
                {
                    // Keep even though empty so that vag index is still correct
                    mSamples.emplace_back(SampleData{ std::vector<u8>() });
                }
            }
        }
    }
}

void Vab::ReadVh(Oddlib::IStream& stream, bool isPsx)
{
    mHeader.Read(stream);

    for (ProgAtr& prog : mProgs)
    {
        prog.Read(stream);
    }

    for (auto i = 0; i < mHeader.iNumProgs; i++)
    {
        for (auto j = 0; j < 16; j++) // 16 = max tones
        {
            auto vagAttr = std::make_unique<VagAtr>(stream);
            mProgs[vagAttr->iProg].iTones.push_back(vagAttr.get());
            mTones.emplace_back(std::move(vagAttr));
        }
    }

    if (isPsx)
    {
        // VAG offset table..
        u32 totalOffset = 0;
        mVagOffsets.reserve(mHeader.iNumVags);
        for (int i = 0; i < mHeader.iNumVags; i++)
        {
            u16 voff = 0;
            stream.ReadUInt16(voff);
            totalOffset += voff << 3;
            mVagOffsets.push_back(totalOffset);
        }
    }
}
