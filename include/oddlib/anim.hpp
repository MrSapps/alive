#pragma once

#include <vector>
#include <memory>
#include <set>
#include <map>
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

    void DebugDumpAnimationFrames(const std::string& fileName, Uint32 id, IStream& stream, bool bIsPsx, const char* dataSetName);

    class AnimSerializer
    {
    public:
        struct FrameHeader
        {
            Uint32 mClutOffset;
            Uint8 mWidth;
            Uint8 mHeight;
            Uint8 mColourDepth;
            Uint8 mCompressionType;
            Uint32 mFrameDataSize; // Actually 2 Uint16's in AE for W/H again
        };
        static_assert(sizeof(FrameHeader) == 12, "Wrong frame header size");

    public:
        AnimSerializer(IStream& stream, bool bIsPsx);
        AnimSerializer(const AnimSerializer&) = delete;
        AnimSerializer& operator = (const AnimSerializer&) = delete;

        SDL_SurfacePtr ApplyPalleteToFrame(const FrameHeader& header, Uint32 realWidth, const std::vector<Uint8>& decompressedData, std::vector<Uint32>& pixels);
        const std::set< Uint32 >& UniqueFrames() const { return mUniqueFrameHeaderOffsets; }
        Uint32 MaxW() const { return mHeader.mMaxW; }
        Uint32 MaxH() const { return mHeader.mMaxH; }

        struct DecodedFrame
        {
            std::vector<Uint8> mPixelData;
            FrameHeader mFrameHeader;
            Uint32 mFixedWidth = 0;
        };
        DecodedFrame ReadAndDecompressFrame(Uint32 frameOffset);
        bool IsSingleFrame() const { return mSingleFrameOffset > 0; }

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

        struct FrameInfoHeader
        {
            Uint32 mFrameHeaderOffset = 0;
            Uint32 mMagic = 0;

            // Top left
            Sint16 mColx = 0;
            Sint16 mColy = 0;

            // Bottom right
            Sint16 mColw = 0;
            Sint16 mColh = 0;

            Sint16 mOffx = 0;
            Sint16 mOffy = 0;
        };
        static_assert(sizeof(FrameInfoHeader) == 20, "Wrong frame info header size");

        const std::vector<std::unique_ptr<AnimationHeader>>& Animations() const { return mAnimationHeaders; }
    private:
        Uint32 GetPaltValue(Uint32 idx);
        Uint32 ParsePallete();
        void ParseAnimationSets();
        void ParseFrameInfoHeaders();
        void GatherUniqueFrameOffsets();
        
        Uint32 mSingleFrameOffset = 0;
        bool mIsPsx = false;

        struct BanHeader
        {
            Uint16 mMaxW = 0;       // Max frame W
            Uint16 mMaxH = 0;       // Max frame H
            Uint32 mFrameTableOffSet = 0; // Where the frame table begins
            Uint32 mPaltSize = 0;         // Number of palt words
        };
        BanHeader mHeader;


        // Unique combination of frames from all animations, as each animation can reuse any number of frames
        std::set< Uint32 > mUniqueFrameHeaderOffsets;


        Uint32 mClutOffset = 0;

        std::vector<std::unique_ptr<AnimationHeader>> mAnimationHeaders;
        std::vector<Uint32> mPalt;

        bool mbIsAoFile = true;

        template<class T>
        std::vector<Uint8> Decompress(FrameHeader& header, Uint32 finalW);
        IStream& mStream;
    };

    class DebugAnimationSpriteSheet
    {
    public:
        DebugAnimationSpriteSheet(AnimSerializer& as, const std::string& fileName, Uint32 id, const char* dataSetName);
    private:
        void DebugDecodeAllFrames(AnimSerializer& as);
        void BeginFrames(int w, int h, int count);
        void AddFrame(AnimSerializer& as, AnimSerializer::DecodedFrame& df, Uint32 offsetData);
        void EndFrames();
        SDL_SurfacePtr mSpriteSheet;
        int mSpritesX = 0;
        int mSpritesY = 0;
        int mXPos = 0;
        int mYPos = 0;

        std::string mFileName;
        std::string mDataSetName;
        Uint32 mId = 0;
    };


    class Animation
    {
    public:
        Animation(const AnimSerializer::AnimationHeader& animHeader, const AnimationSet& animSet);

        struct Frame
        {
            // Frame offset for correct positioning
            int mOffX;
            int mOffY;

            // Bounding box
            int mBX;
            int mBY;
            int mBW;
            int mBH;

            // Image pixel data - pointer as data is sometimes shared between frames
            SDL_Surface* mFrame;
        };
        Uint32 NumFrames() const { return static_cast<Uint32>(mFrames.size()); }
        Uint32 Fps() const { return mFps; }
        Uint32 LoopStartFrame() const { return mLoopStartFrame; }
        const Frame& GetFrame(Uint32 idx) const;
    private:
        Uint32 mFps = 0;
        Uint32 mLoopStartFrame = 0;
        std::vector<Frame> mFrames;
    };


    class AnimationSet
    {
    public:
        explicit AnimationSet(AnimSerializer& as);
        Uint32 NumberOfAnimations() const;
        const Animation* AnimationAt(Uint32 idx) const;
        SDL_Surface* FrameByOffset(Uint32 offset) const;
        Uint32 MaxW() const { return mMaxW; }
        Uint32 MaxH() const { return mMaxH; }
    private:
        SDL_SurfacePtr MakeFrame(AnimSerializer& as, const AnimSerializer::DecodedFrame& df, Uint32 offsetData);

        std::vector<std::unique_ptr<Animation>> mAnimations;

        // Map of frame offsets to frame images
        std::map<Uint32, SDL_SurfacePtr> mFrames;

        Uint32 mMaxW = 0;
        Uint32 mMaxH = 0;
    };

}
