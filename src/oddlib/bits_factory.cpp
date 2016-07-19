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
    static bool IsAePsxCam(IStream& stream)
    {
        Uint16 size = 0;
        stream.ReadUInt16(size);

        Uint16 nulls = 0;
        stream.ReadUInt16(nulls);
        if (nulls != 0)
        {
            return false;
        }

        Uint16 temp = 0;
        stream.ReadUInt16(temp);

        // [00 38] [xx xx] [02 00]
        stream.ReadUInt16(temp);
        if (temp != 0x3800)
        {
            return false;
        }

        stream.ReadUInt16(temp);

        stream.ReadUInt16(temp);
        if (temp != 0x0002)
        {
            return false;
        }

        return true;
    }

    static bool IsAoDemoPsxCam(IStream& stream)
    {
        // Look for a pattern of [00 38]
        Uint16 size = 0;
        stream.ReadUInt16(size);

        Uint16 temp = 0;
        stream.ReadUInt16(temp);
        if (temp != 0x3800)
        {
            return false;
        }

        return true;
    }

    enum eCameraType
    {
        eAoPsxDemo,
        eAePsx,
        eAoPsx,
        eAoPc,
        eAePc
    };

    static eCameraType GetCameraType(IStream& stream)
    {
        bool allStripsAreAoSize = true;
        bool hasFullAmountOfStrips = true;
        const auto aoStripSize = 16 * 240 * 2;
        const auto kNumStrips = 640 / 16;

        const auto streamSize = stream.Size();

        if (IsAoDemoPsxCam(stream))
        {
            LOG_INFO("AO PSX demo mdec camera detected");
            stream.Seek(0);
            return eAoPsxDemo;
        }

        stream.Seek(0);

        if (IsAePsxCam(stream))
        {
            stream.Seek(0);
            LOG_INFO("AE PSX mdec camera detected");
            return eAePsx;
        }

        stream.Seek(0);
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
            LOG_INFO("AO PSX mdec camera detected");
            return eAoPsx;
        }
        else if (allStripsAreAoSize)
        {
            LOG_INFO("AO PC camera detected");
            return eAoPc;
        }
        LOG_INFO("AE PC camera detected");
        return eAePc;
    }

    bool IsPsxCamera(IStream& stream)
    {
        const eCameraType cameraType = GetCameraType(stream);
        switch (cameraType)
        {
            case eAoPsxDemo: return true;
            case eAePsx:     return true;
            case eAoPsx:     return true;
            case eAoPc:      return false;
            case eAePc:      return false;
        }
        abort();
    }

    std::unique_ptr<IBits> MakeBits(IStream& stream, std::shared_ptr<Oddlib::LvlArchive>& lvl)
    {
        const eCameraType cameraType = GetCameraType(stream);
        switch (cameraType)
        {
        case eAoPsxDemo: return std::make_unique<PsxBits>(stream, false, true, lvl);
        case eAePsx:     return std::make_unique<PsxBits>(stream, true, false, lvl);
        case eAoPsx:     return std::make_unique<PsxBits>(stream, false, false, lvl);
        case eAoPc:      return std::make_unique<AoBitsPc>(stream, lvl);
        case eAePc:      return std::make_unique<AeBitsPc>(stream, lvl);
        }
        abort();
    }
}
