#include "oddlib/bits_factory.hpp"
#include "oddlib/bits_fg1.hpp"
#include "oddlib/stream.hpp"
#include "oddlib/bits_ao_pc.hpp"
#include "oddlib/bits_ae_pc.hpp"
#include "oddlib/bits_psx.hpp"
#include "logger.hpp"
#include <array>
#include <fstream>

namespace Oddlib
{
    static bool IsAePsxCam(IStream& stream)
    {
        u16 size = 0;
        stream.Read(size);

        u16 nulls = 0;
        stream.Read(nulls);
        if (nulls != 0)
        {
            return false;
        }

        u16 temp = 0;
        stream.Read(temp);

        // [00 38] [xx xx] [02 00]
        stream.Read(temp);
        if (temp != 0x3800)
        {
            return false;
        }

        stream.Read(temp);

        stream.Read(temp);
        if (temp != 0x0002)
        {
            return false;
        }

        return true;
    }

    static bool IsAoDemoPsxCam(IStream& stream)
    {
        // Look for a pattern of [00 38]
        u16 size = 0;
        stream.Read(size);

        u16 temp = 0;
        stream.Read(temp);
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

            u16 stripSize = 0;
            stream.Read(stripSize);
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

    class Bits : public IBits
    {
    public:
        Bits(SDL_SurfacePtr camImage)
            : mCameraImage(std::move(camImage))
        {

        }

        virtual SDL_Surface* GetSurface() const override
        {
            return mCameraImage.get();
        }

        virtual IFg1* GetFg1() const override { return nullptr; }

    private:
        SDL_SurfacePtr mCameraImage;
    };

    std::unique_ptr<IBits> MakeBits(SDL_SurfacePtr camImage)
    {
        return std::make_unique<Bits>(std::move(camImage));
    }

    std::unique_ptr<IBits> MakeBits(IStream& bitsStream, IStream* fg1Stream)
    {
        const eCameraType cameraType = GetCameraType(bitsStream);
        switch (cameraType)
        {
        case eAoPsxDemo: return std::make_unique<PsxBits>(bitsStream, false, true, fg1Stream);
        case eAePsx:     return std::make_unique<PsxBits>(bitsStream, true, false, fg1Stream);
        case eAoPsx:     return std::make_unique<PsxBits>(bitsStream, false, false, fg1Stream);
        case eAoPc:      return std::make_unique<AoBitsPc>(bitsStream, fg1Stream);
        case eAePc:      return std::make_unique<AeBitsPc>(bitsStream, fg1Stream);
        }
        abort();
    }

    IBits::~IBits()
    {

    }

    void IBits::Save()
    {
        static int i = 1;
        SDL_SaveBMP(GetSurface(), ("camera" + std::to_string(i++) + ".bmp").c_str());
    }

    IFg1::~IFg1()
    {

    }

    void IFg1::Save(const std::string& baseName)
    {
        static int i = 1;
        SDLHelpers::SaveSurfaceAsPng((baseName + "_camera_fg1" + std::to_string(i++) + ".png").c_str(), GetSurface());
    }

}
