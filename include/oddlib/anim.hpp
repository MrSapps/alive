#pragma once

#include <vector>
#include <memory>
#include "SDL.h"

namespace Oddlib
{
    class LvlArchive;
    class IStream;

    class AnimSerializer
    {
    public:
        AnimSerializer(IStream& stream);
    private:
        void ParseAnimationSets(IStream& stream);

        struct BanHeader
        {
            Uint16 iMaxW = 0;       // Max frame W
            Uint16 iMaxH = 0;       // Max frame H
            Uint32 iFrameTableOffSet = 0; // Where the frame table begins
            Uint32 iPaltSize = 0;         // Number of palt words

        };
        BanHeader h;

        struct FrameInfoHeader;
        struct AnimationHeader
        {
            Uint16 iFps = 0;            // Seems to be 0x1 or 0x2
            Uint16 iNumFrames = 0;      // Number of frames in the set

            // If loop flag set then this is the frame to loop back to
            Uint16 iLoopStartFrame = 0;
            
            // Bit flags, bit 1 = never unload?, bit 2 = loop
            Uint16 iFlags = 0;

            // Offset to each frame, can be duplicated across sets if two animations share the same frame
            std::vector< Uint32 > iFrameInfoOffsets;

            std::vector<std::unique_ptr<FrameInfoHeader>> mFrameInfos;
        };

        struct FrameInfoHeader
        {
            Uint32 iFrameHeaderOffset = 0;
            Uint32 iMagic = 0;

            // TODO: Actually, number of points and triggers?
            //Uint16 points = 0;
            //Uint16 triggers = 0;

            // Top left
            Sint16 colx = 0;
            Sint16 coly = 0;

            // Bottom right
            Sint16 colw = 0;
            Sint16 colh = 0;

            Sint16 offx = 0;
            Sint16 offy = 0;

            std::vector<Uint32> mTriggers;
        };

        struct FrameHeader
        {

        };

        std::vector<std::unique_ptr<AnimationHeader>> mAnimationHeaders;

        bool mbIsAoFile = true;
    };

    class Frame
    {
    public:
    };

    class Animation
    {
    public:
        std::vector<std::shared_ptr<Frame>> mFrames;
    };

    class AnimationFactory
    {
    public:
        static std::vector<std::unique_ptr<Animation>> Create(Oddlib::LvlArchive& archive, const std::string& fileName, Uint32 resourceId);
    };
}
