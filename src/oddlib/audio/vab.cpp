#include "oddlib/audio/vab.hpp"
#include "oddlib/PSXADPCMDecoder.h"
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <map>
#include <stdlib.h>
#include <string.h>

void Vab::ReadVb(Oddlib::IStream& s)
{

    // TODO: Some offsets might be duplicated
    for (PsxVag& vag : mVagOffsets)
    {

        s.Seek(vag.mOffset);
        PSXADPCMDecoder d;
        d.DecodeVagStream(s, vag.iSampleData);

        /*
        static int i = 0;
        std::string name = "raw_" + std::to_string(i++);
        std::ofstream o(name.c_str(), std::ios::binary);
        o.write((const char*)vag.iSampleData.data(), vag.iSampleData.size());
        */
    }

    /*
    auto streamSize = s.Size();
    if (streamSize > 5120) // HACK: No exoddus vb is greater than 5kb
    {
        for (auto i = 0; i < mHeader.iNumVags; ++i)
        {
            auto vag = std::make_unique<AoVag>();
            s.ReadUInt32(vag->iSize);
            s.ReadUInt32(vag->iSampleRate);

            vag->iSampleData.resize(vag->iSize);
            s.ReadBytes(vag->iSampleData.data(), vag->iSampleData.size());
            iAoVags.emplace_back(std::move(vag));
        }
    }
    else
    {
        for (unsigned int i = 0; i < mHeader.iNumVags; i++)
        {
            iOffs.emplace_back(std::make_unique<AEVh>(s));
        }
    }*/
}

void Vab::ReadVh(Oddlib::IStream& stream)
{
    mHeader.Read(stream);
    int tones = 0;
    for (auto i = 0; i < 128; i++) // 128 = max progs
    {
        mProgs[i].Read(stream);
        tones += mProgs[i].iNumTones;
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

    // VAG offset table..
    Uint32 totalOffset = 0;
    mVagOffsets.reserve(mHeader.iNumVags);
    for (int i = 0; i < mHeader.iNumVags; i++)
    {
        PsxVag tmp;
        Uint16 voff = 0;
        stream.ReadUInt16(voff);
        totalOffset += voff << 3;
        tmp.mOffset = totalOffset;
        mVagOffsets.push_back(tmp);
    }
}
