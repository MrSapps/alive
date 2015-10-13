#include "oddlib/bits_factory.hpp"
#include "oddlib/stream.hpp"
#include "oddlib/ao_bits_pc.hpp"
#include "oddlib/ae_bits_pc.hpp"
#include "logger.hpp"

namespace Oddlib
{
    std::unique_ptr<IBits> MakeBits(IStream& stream)
    {
        bool allStripsAreAoSize = true;
        const auto aoStripSize = 16 * 240 * 2;
        const auto kNumStrips = 640 / 16;
        for (auto i = 0; i < kNumStrips; i++)
        {
            Uint16 stripSize = 0;
            stream.ReadUInt16(stripSize);
            if (stripSize != aoStripSize)
            {
                allStripsAreAoSize = false;
                break;
            }
            stream.Seek(stream.Pos() + stripSize);
        }

        stream.Seek(0);
        if (allStripsAreAoSize)
        {
            LOG_INFO("AO PC camera detected");
            return std::make_unique<AoBitsPc>(stream);
        }
        LOG_INFO("AE PC camera detected");
        return std::make_unique<AeBitsPc>(stream);
    }
}
