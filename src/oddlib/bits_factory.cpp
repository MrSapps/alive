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
        bool allStripsAreAoSize = true;
        bool hasFullAmountOfStrips = true;
        const auto aoStripSize = 16 * 240 * 2;
        const auto kNumStrips = 640 / 16;

        const auto streamSize = stream.Size();
        for (auto i = 0; i < kNumStrips; i++)
        {
            // PSX cams are half the resolution, so if we bail before getting to kNumStrips
            // then must be PSX data.
            if (stream.Pos() >= streamSize)
            {
                hasFullAmountOfStrips = false;
                break;
            }

            Uint16 stripSize = 0;
            stream.ReadUInt16(stripSize);
            if (stripSize != aoStripSize)
            {
                allStripsAreAoSize = false;
            }

            stream.Seek(stream.Pos() + stripSize);
        }
       
        stream.Seek(0);

        if (!hasFullAmountOfStrips)
        {
            LOG_INFO("PSX mdec camera detected");
            return std::make_unique<PsxBits>(stream);
        }
        else if (allStripsAreAoSize)
        {
            LOG_INFO("AO PC camera detected");
            return std::make_unique<AoBitsPc>(stream);
        }
        LOG_INFO("AE PC camera detected");
        return std::make_unique<AeBitsPc>(stream);
    }
}
