#include "oddlib/bits_factory.hpp"
#include "oddlib/stream.hpp"
#include "oddlib/ao_bits_pc.hpp"
#include "oddlib/ae_bits_pc.hpp"
#include "oddlib/psx_bits.hpp"
#include "logger.hpp"
#include <array>
#include <fstream>

namespace Oddlib
{
    std::unique_ptr<IBits> MakeBits(IStream& stream)
    {
        bool allStripsAreMdecHeader = true;
        bool allStripsAreAoSize = true;
        const auto aoStripSize = 16 * 240 * 2;
        const auto kNumStrips = 640 / 16;
        std::array<Uint8, 4> mdecHeader;

        std::vector<Uint8> b(stream.Size());
        stream.ReadBytes(b.data(), b.size());

        std::ofstream o("debug.dat", std::ios::binary);
        o.write((const char*)b.data(), b.size());
        o.close();

        for (auto i = 0; i < kNumStrips; i++)
        {
            Uint16 stripSize = 0;
            stream.ReadUInt16(stripSize);
            auto previousPos = stream.Pos();

            // If we found a strip isn't an mdec header then don't check the rest of them
            if (allStripsAreMdecHeader)
            {
                // Look for a pattern of [XX XX] [00 38] 
                // if this is found then assume its an mdec header
                stream.ReadBytes(mdecHeader.data(), mdecHeader.size());
                if (mdecHeader[2] != 0x0 || mdecHeader[3] != 0x38 )
                {
                    allStripsAreMdecHeader = false;
                }
            }

            if (stripSize != aoStripSize)
            {
                allStripsAreAoSize = false;
            }

            stream.Seek(previousPos + stripSize);
        }

        stream.Seek(0);
        if (allStripsAreMdecHeader)
        {
            LOG_INFO("PSX mdec camera detected");
            return std::make_unique<PsxBits>(stream);
        }
        else if (allStripsAreAoSize)
        {
            abort();
            LOG_INFO("AO PC camera detected");
            return std::make_unique<AoBitsPc>(stream);
        }
        abort();
        LOG_INFO("AE PC camera detected");
        return std::make_unique<AeBitsPc>(stream);
    }
}
