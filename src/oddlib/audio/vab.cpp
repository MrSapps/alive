#include "oddlib/audio/vab.hpp"
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <map>
#include <stdlib.h>
#include <string.h>

Vab::Vab()
{

}

Vab::Vab(std::string aVhFile, std::string aVbFile)
{
    LoadVhFile(aVhFile);
    LoadVbFile(aVbFile);
}

void Vab::LoadVbFile(std::string aFileName)
{
    // Read the file.
    std::ifstream file;
    file.open(aFileName.c_str(), std::ios::binary);
    if (!file.is_open())
    {
        abort();
    }
    ReadVb(file);
}

void Vab::ReadVb(std::istream& aStream)
{
    aStream.seekg(0, aStream.end);
    auto streamSize = aStream.tellg();
    aStream.seekg(0, aStream.beg);

    if (streamSize > 5120) // HACK: No exoddus vb is greater than 5kb
    {
        for (auto i = 0; i < iHeader->iNumVags; ++i)
        {
            auto vag = std::make_unique<AoVag>();
            aStream.read((char*)&vag->iSize, sizeof(vag->iSize));
            aStream.read((char*)&vag->iSampleRate, sizeof(vag->iSampleRate));
            vag->iSampleData.resize(vag->iSize);
            aStream.read((char*)vag->iSampleData.data(), vag->iSize);

            iAoVags.emplace_back(std::move(vag));
        }
    }
    else
    {
        for (unsigned int i = 0; i < iHeader->iNumVags; i++)
        {
            iOffs.emplace_back(std::make_unique<AEVh>(aStream));
        }
    }
}


void Vab::LoadVhFile(std::string aFileName)
{
    // Read the file.
    std::ifstream file;
    file.open(aFileName.c_str(), std::ios::binary);
    if (!file.is_open())
    {
        abort();
    }
    ReadVh(file);
}

void Vab::ReadVh(std::istream& aStream)
{
    iHeader = new VabHeader(aStream);
    int tones = 0;
    for (unsigned int i = 0; i < 128; i++) // 128 = max progs
    {
        auto progAttr = std::make_unique<ProgAtr>(aStream);
        tones += progAttr->iNumTones;
        mProgs.emplace_back(std::move(progAttr));
    }

    for (unsigned int i = 0; i < iHeader->iNumProgs; i++)
    {
        for (unsigned int j = 0; j < 16; j++) // 16 = max tones
        {
            auto vagAttr = std::make_unique<VagAtr>(aStream);
            mProgs[vagAttr->iProg]->iTones.push_back(vagAttr.get());
            mTones.emplace_back(std::move(vagAttr));
        }
    }
}
