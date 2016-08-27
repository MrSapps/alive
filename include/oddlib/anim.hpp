#pragma once

#include <vector>
#include <memory>
#include <set>
#include <map>
#include "SDL.h"
#include "sdl_raii.hpp"
#include <string>
#include "types.hpp"

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
    *
    * However AE data has an extra 0 DWORD before the palt size - in the PSX data
    * sometimes it isn't 0. In this case the format is that there is only one large
    * "sprite sheet" frame, and each frame infos is a sub rect of the larger image.
    *
    * Note: Because compression type 6 is used in AE PSX and AE PC, we have to pass in a flag to
    * know if the source data is PC or PSX. This is because the type 6 decompression is not the same between
    * PC and PSX, and there is no reliable way to detect which format we have.
    */
    class AnimSerializer;
    class AnimationSet;

    std::unique_ptr<AnimationSet> LoadAnimations(IStream& stream, bool bIsPsx);

    void DebugDumpAnimationFrames(const std::string& fileName, u32 id, IStream& stream, bool bIsPsx, const char* dataSetName);

    class AnimSerializer
    {
    public:
        struct FrameHeader
        {
            u32 mClutOffset;
            u8 mWidth;
            u8 mHeight;
            u8 mColourDepth;
            u8 mCompressionType;
            u32 mFrameDataSize; // Actually 2 u16's in AE for W/H again
        };
        static_assert(sizeof(FrameHeader) == 12, "Wrong frame header size");

    public:
        AnimSerializer(IStream& stream, bool bIsPsx);
        AnimSerializer(const AnimSerializer&) = delete;
        AnimSerializer& operator = (const AnimSerializer&) = delete;

        SDL_SurfacePtr ApplyPalleteToFrame(const FrameHeader& header, u32 realWidth, const std::vector<u8>& decompressedData, std::vector<u32>& pixels);
        const std::set< u32 >& UniqueFrames() const { return mUniqueFrameHeaderOffsets; }
        u32 MaxW() const { return mHeader.mMaxW; }
        u32 MaxH() const { return mHeader.mMaxH; }

        struct DecodedFrame
        {
            std::vector<u8> mPixelData;
            FrameHeader mFrameHeader;
            u32 mFixedWidth = 0;
        };
        DecodedFrame ReadAndDecompressFrame(u32 frameOffset);
        bool IsSingleFrame() const { return mSingleFrameOffset > 0; }

        struct FrameInfoHeader;
        struct AnimationHeader
        {
            u16 mFps = 0;            // Seems to be 0x1 or 0x2
            u16 mNumFrames = 0;      // Number of frames in the set

            // If loop flag set then this is the frame to loop back to
            u16 mLoopStartFrame = 0;

            enum eFlags
            {
                eFlipXFlag = 0x4,
                eFlipYFlag = 0x8,
                eNeverUnload = 0x1,
                eLoopFlag = 0x2
            };
            u16 mFlags = 0;

            // Offset to each frame, can be duplicated across sets if two animations share the same frame
            std::vector< u32 > mFrameInfoOffsets;

            std::vector<std::unique_ptr<FrameInfoHeader>> mFrameInfos;
        };

        struct Point
        {
            s16 x = 0;
            s16 y = 0;
        };

        struct FrameInfoHeader
        {
            u32 mFrameHeaderOffset = 0;
            u32 mMagic = 0;

            // Collision bounding rectangle
            Point mTopLeft;
            Point mBottomRight;

            s16 mOffx = 0;
            s16 mOffy = 0;
        };
        static_assert(sizeof(FrameInfoHeader) == 20, "Wrong frame info header size");

        const std::vector<std::unique_ptr<AnimationHeader>>& Animations() const { return mAnimationHeaders; }
    private:
        u32 GetPaltValue(u32 idx);
        u32 ParsePallete();
        void ParseAnimationSets();
        void ParseFrameInfoHeaders();
        void GatherUniqueFrameOffsets();
        
        u32 mSingleFrameOffset = 0;
        bool mIsPsx = false;

        struct BanHeader
        {
            u16 mMaxW = 0;       // Max frame W
            u16 mMaxH = 0;       // Max frame H
            u32 mFrameTableOffSet = 0; // Where the frame table begins
            u32 mPaltSize = 0;         // Number of palt words
        };
        BanHeader mHeader;


        // Unique combination of frames from all animations, as each animation can reuse any number of frames
        std::set< u32 > mUniqueFrameHeaderOffsets;


        u32 mClutOffset = 0;

        std::vector<std::unique_ptr<AnimationHeader>> mAnimationHeaders;
        std::vector<u32> mPalt;

        bool mbIsAoFile = true;

        template<class T>
        std::vector<u8> Decompress(FrameHeader& header, u32 finalW);
        IStream& mStream;
    };

    class DebugAnimationSpriteSheet
    {
    public:
        DebugAnimationSpriteSheet(AnimSerializer& as, const std::string& fileName, u32 id, const char* dataSetName);
    private:
        void DebugDecodeAllFrames(AnimSerializer& as);
        void BeginFrames(int w, int h, int count);
        void AddFrame(AnimSerializer& as, AnimSerializer::DecodedFrame& df, u32 offsetData);
        void EndFrames();
        SDL_SurfacePtr mSpriteSheet;
        int mSpritesX = 0;
        int mSpritesY = 0;
        int mXPos = 0;
        int mYPos = 0;

        std::string mFileName;
        std::string mDataSetName;
        u32 mId = 0;
    };


    class Animation
    {
    public:
        Animation(const AnimSerializer::AnimationHeader& animHeader, const AnimationSet& animSet);

        struct BoundingBoxPoint
        {
            s32 x;
            s32 y;
        };

        struct Frame
        {
            // Frame offset for correct positioning
            s32 mOffX;
            s32 mOffY;

            // Bounding box
            BoundingBoxPoint mTopLeft;
            BoundingBoxPoint mBottomRight;

            // Image pixel data - pointer as data is sometimes shared between frames
            SDL_Surface* mFrame;
        };

        u32 NumFrames() const { return static_cast<u32>(mFrames.size()); }
        u32 Fps() const { return mFps; }
        u32 LoopStartFrame() const { return mLoopStartFrame; }
        bool Loop() const { return mbLoop; }
        bool FlipX() const { return mbFlipX; }
        bool FlipY() const { return mbFlipY; }
        const Frame& GetFrame(u32 idx) const;
    private:
        u32 mFps = 0;
        u32 mLoopStartFrame = 0;
        std::vector<Frame> mFrames;
        bool mbLoop = false;
        bool mbFlipX = false;
        bool mbFlipY = false;
    };


    class AnimationSet
    {
    public:
        explicit AnimationSet(AnimSerializer& as);
        u32 NumberOfAnimations() const;
        const Animation* AnimationAt(u32 idx) const;
        SDL_Surface* FrameByOffset(u32 offset) const;
        u32 MaxW() const { return mMaxW; }
        u32 MaxH() const { return mMaxH; }
    private:
        SDL_SurfacePtr MakeFrame(AnimSerializer& as, const AnimSerializer::DecodedFrame& df, u32 offsetData);

        std::vector<std::unique_ptr<Animation>> mAnimations;

        // Map of frame offsets to frame images
        std::map<u32, SDL_SurfacePtr> mFrames;

        u32 mMaxW = 0;
        u32 mMaxH = 0;
    };

}
