#pragma once

#include <vector>
#include <memory>
#include <set>
#include "SDL.h"
#include "sdl_raii.hpp"
#include <string>

namespace Oddlib
{
    class LvlArchive;
    class IStream;
    /*
    * Typical animation file structure :
    * ResHeader
    * Offset to table of AnimSets
    * Max frame W
    * Max frame H
    * Palt size
    * Palt data
    * Frames(FrameHeader + data)
    * AnimSets - an array of offsets / pointers to FrameInfos
    * FrameInfos - offsets / pointers to FrameHeaders
    * EOF
    */
    class AnimSerializer
    {
    public:
        explicit AnimSerializer(const std::string& fileName, Uint32 id, IStream& stream);
    private:
        void ParseAnimationSets(IStream& stream);
        void ParseFrameInfoHeaders(IStream& stream);
        void GatherUniqueFrameOffsets();
        Uint32 DataSize(std::set<Uint32>::iterator it);
        void DebugDecodeAllFrames(IStream& stream);
        std::vector<Uint8> DecodeFrame(IStream& stream, Uint32 frameOffset, Uint32 frameDataSize);
        
        std::string mFileName;
        Uint32 mId = 0;

        struct BanHeader
        {
            Uint16 mMaxW = 0;       // Max frame W
            Uint16 mMaxH = 0;       // Max frame H
            Uint32 mFrameTableOffSet = 0; // Where the frame table begins
            Uint32 mPaltSize = 0;         // Number of palt words

        };
        BanHeader mHeader;

        struct FrameInfoHeader;
        struct AnimationHeader
        {
            Uint16 mFps = 0;            // Seems to be 0x1 or 0x2
            Uint16 mNumFrames = 0;      // Number of frames in the set

            // If loop flag set then this is the frame to loop back to
            Uint16 mLoopStartFrame = 0;
            
            enum eFlags
            {
                eFlipXFlag = 0x4,
                eFlipYFlag = 0x8,
                eNeverUnload = 0x1,
                eLoopFlag = 0x2
            };
            Uint16 mFlags = 0;

            // Offset to each frame, can be duplicated across sets if two animations share the same frame
            std::vector< Uint32 > mFrameInfoOffsets;

            std::vector<std::unique_ptr<FrameInfoHeader>> mFrameInfos;
        };

        // Unique combination of frames from all animations, as each animation can reuse any number of frames
        std::set< Uint32 > mUniqueFrameHeaderOffsets;

        struct FrameInfoHeader
        {
            Uint32 mFrameHeaderOffset = 0;
            Uint32 mMagic = 0;

            // TODO: Actually, number of points and triggers?
            //Uint16 points = 0;
            //Uint16 triggers = 0;

            // Top left
            Sint16 mColx = 0;
            Sint16 mColy = 0;

            // Bottom right
            Sint16 mColw = 0;
            Sint16 mColh = 0;

            Sint16 mOffx = 0;
            Sint16 mOffy = 0;

            //std::vector<Uint32> mTriggers;
        };

        struct FrameHeader
        {
            Uint32 mMagic;          // Always 0x8 for AO and AE also is the offset to palt/clut?
            Uint8 mWidth;
            Uint8 mHeight;
            Uint8 mColourDepth;
            Uint8 mCompressionType;
            Uint32 mFrameDataSize; // Actually 2 Uint16's in AE for W/H again
        };
        static_assert(sizeof(FrameHeader) == 12, "Wrong frame header size");

        std::vector<std::unique_ptr<AnimationHeader>> mAnimationHeaders;
        std::vector<Uint16> mPalt;

        bool mbIsAoFile = true;

        template<class T>
        std::vector<Uint8> Decompress(FrameHeader& header, IStream& stream, Uint32 finalW, Uint32 w, Uint32 h, Uint32 dataSize);

        // TODO: Put this stuff into its own object
        void BeginFrames(int w, int h, int count);
        void AddFrame(FrameHeader& header, Uint32 realWidth, const std::vector<Uint8>& decompressedData);
        void EndFrames();
        void DebugSaveFrame(FrameHeader& header, Uint32 realWidth, const std::vector<Uint8>& decompressedData);
        SDL_SurfacePtr MakeFrame(FrameHeader& header, Uint32 realWidth, const std::vector<Uint8>& decompressedData, std::vector<Uint16>& pixels);
        SDL_SurfacePtr mSpriteSheet;
        int mSpritesX = 0;
        int mSpritesY = 0;
        int mSpriteX = 0;
        int mSpriteY = 0;
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
