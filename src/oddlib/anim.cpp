#include "oddlib/anim.hpp"
#include "oddlib/lvlarchive.hpp"
#include "oddlib/stream.hpp"
#include "oddlib/compressiontype2.hpp"
#include "oddlib/compressiontype3.hpp"
#include "oddlib/compressiontype3ae.hpp"
#include "oddlib/compressiontype4or5.hpp"
#include "oddlib/compressiontype6ae.hpp"
#include "oddlib/compressiontype6or7aepsx.hpp"
#include "logger.hpp"
#include "sdl_raii.hpp"
#include <assert.h>
#include <array>

namespace Oddlib
{
    Animation::Animation(const AnimSerializer::AnimationHeader& animHeader, const AnimationSet& animSet)
    {
        mFps = animHeader.mFps;
        mLoopStartFrame = animHeader.mLoopStartFrame;

        mbLoop = (animHeader.mFlags & AnimSerializer::AnimationHeader::eLoopFlag) > 0;
        mbFlipX = (animHeader.mFlags & AnimSerializer::AnimationHeader::eFlipXFlag) > 0;
        mbFlipY = (animHeader.mFlags & AnimSerializer::AnimationHeader::eFlipYFlag) > 0;

        for (const std::unique_ptr<AnimSerializer::FrameInfoHeader>& frameInfo : animHeader.mFrameInfos)
        {
            Frame tmp;

            // Frame image
            tmp.mFrame = animSet.FrameByOffset(frameInfo->mFrameHeaderOffset);

            // Frame offset so animation "looks" correct
            tmp.mOffX = frameInfo->mOffx;
            tmp.mOffY = frameInfo->mOffy;

            // Collision bounding rectangle
            tmp.mTopLeft.x = frameInfo->mTopLeft.x;
            tmp.mTopLeft.y = frameInfo->mTopLeft.y;
            tmp.mBottomRight.x = frameInfo->mBottomRight.x;
            tmp.mBottomRight.y = frameInfo->mBottomRight.y;
            mFrames.push_back(tmp);
        }
    }

    const Animation::Frame& Animation::GetFrame(u32 idx) const
    {
        return mFrames[idx];
    }

    AnimationSet::AnimationSet(AnimSerializer& as)
    {
        mMaxW = as.MaxW();
        mMaxH = as.MaxH();

        // Add all frames
        for (auto it : as.UniqueFrames())
        {
            const AnimSerializer::DecodedFrame decoded = as.ReadAndDecompressFrame(it);
            mFrames[it] = MakeFrame(as, decoded, it);
        }

        // Add animations that point to the frames
        for (const std::unique_ptr<AnimSerializer::AnimationHeader>& animSet : as.Animations())
        {
            // Some animations have 0 frames for some bizzare reason
            if (animSet->mFrameInfos.empty() == false)
            {
                mAnimations.push_back(std::make_unique<Animation>(*animSet, *this));
            }
        }
    }

    SDL_SurfacePtr AnimationSet::MakeFrame(AnimSerializer& as, const AnimSerializer::DecodedFrame& df, u32 offsetData)
    {
        std::vector<u32> pixels;
        auto frame = as.ApplyPalleteToFrame(df.mFrameHeader, df.mFixedWidth, df.mPixelData, pixels);

        SDL_Rect dstRect;
        dstRect.x = 0;
        dstRect.y = 0;
        dstRect.w = df.mFrameHeader.mWidth;
        dstRect.h = df.mFrameHeader.mHeight;

        SDL_Rect srcRect;
        if (as.IsSingleFrame())
        {
            // The anim is one big premade sprite sheet, so we have to "cut out" the rect
            // of the frame we're after

            // SURPRISE! The frame offset is actually a rect in this case
            unsigned char bytes[4];
            bytes[0] = (offsetData >> 24) & 0xFF;
            bytes[1] = (offsetData >> 16) & 0xFF;
            bytes[2] = (offsetData >> 8) & 0xFF;
            bytes[3] = offsetData & 0xFF;


            srcRect.x = bytes[3];
            srcRect.y = bytes[2];
            srcRect.w = bytes[1];
            srcRect.h = bytes[0];


            dstRect.w = srcRect.w;
            dstRect.h = srcRect.h;

        }
        else
        {
            srcRect.x = 0;
            srcRect.y = 0;
            srcRect.w = df.mFrameHeader.mWidth;
            srcRect.h = df.mFrameHeader.mHeight;
        }

        const auto red_mask = 0x000000ff;
        const auto green_mask = 0x0000ff00;
        const auto blue_mask = 0x00ff0000;
        const auto alpha_mask = 0xff000000;
        SDL_SurfacePtr tmp(SDL_CreateRGBSurface(0, dstRect.w, dstRect.h, 32, red_mask, green_mask, blue_mask, alpha_mask));

        SDL_BlitSurface(frame.get(), &srcRect, tmp.get(), &dstRect);
        return tmp;
    }

    u32 AnimationSet::NumberOfAnimations() const
    {
        return static_cast<u32>(mAnimations.size());
    }

    const Animation* AnimationSet::AnimationAt(u32 idx) const
    {
        return mAnimations[idx].get();
    }

    SDL_Surface* AnimationSet::FrameByOffset(u32 offset) const
    {
        auto it = mFrames.find(offset);
        if (it != std::end(mFrames))
        {
            return it->second.get();
        }
        return nullptr;
    }

    std::unique_ptr<AnimationSet> LoadAnimations(IStream& stream, bool bIsPsx)
    {
        AnimSerializer as(stream, bIsPsx);
        return std::make_unique<AnimationSet>(as);
    }

    DebugAnimationSpriteSheet::DebugAnimationSpriteSheet(AnimSerializer& as, const std::string& fileName, u32 id, const char* dataSetName)
        : mFileName(fileName), mDataSetName(dataSetName), mId(id)
    {
        DebugDecodeAllFrames(as);
    }


    void DebugAnimationSpriteSheet::DebugDecodeAllFrames(AnimSerializer& as)
    {
        BeginFrames(as.MaxW(), as.MaxH(), static_cast<int>(as.UniqueFrames().size()));

        for (auto it : as.UniqueFrames())
        {
            auto frameData = as.ReadAndDecompressFrame(it);
            AddFrame(as, frameData, it);
        }

        EndFrames();
    }

    void DebugAnimationSpriteSheet::BeginFrames(int w, int h, int count)
    {
        if (count <= 0)
        {
            count = 1;
        }

        mSpritesX = 10;
        mSpritesY = count / 10;
        if (mSpritesY <= 0)
        {
            mSpritesY = 1;
        }
        if (count < 10)
        {
            mSpritesX = count;
        }

        mXPos = 0;
        mYPos = 0;

        // Flip order of colours around for PNG lib
        const auto red_mask = 0x000000ff;
        const auto green_mask = 0x0000ff00;
        const auto blue_mask = 0x00ff0000;
        const auto alpha_mask = 0xff000000;
        mSpriteSheet.reset(SDL_CreateRGBSurface(0, mSpritesX*w, mSpritesY*h, 32, red_mask, green_mask, blue_mask, alpha_mask));
    }

    void DebugAnimationSpriteSheet::AddFrame(AnimSerializer& as, AnimSerializer::DecodedFrame& df, u32 offsetData)
    {
        int xpos = mXPos * as.MaxW();
        int ypos = mYPos * as.MaxH();

        std::vector<u32> pixels;
        auto frame = as.ApplyPalleteToFrame(df.mFrameHeader, df.mFixedWidth, df.mPixelData, pixels);


        SDL_Rect dstRect;
        dstRect.x = xpos;
        dstRect.y = ypos;
        dstRect.w = df.mFrameHeader.mWidth;
        dstRect.h = df.mFrameHeader.mHeight;

        SDL_Rect srcRect;
        if (as.IsSingleFrame())
        {
            // The anim is one big premade sprite sheet, so we have to "cut out" the rect
            // of the frame we're after

            // SURPRISE! The frame offset is actually a rect in this case
            unsigned char bytes[4];
            bytes[0] = (offsetData >> 24) & 0xFF;
            bytes[1] = (offsetData >> 16) & 0xFF;
            bytes[2] = (offsetData >> 8) & 0xFF;
            bytes[3] = offsetData & 0xFF;


            srcRect.x = bytes[3];
            srcRect.y = bytes[2];
            srcRect.w = bytes[1];
            srcRect.h = bytes[0];


            dstRect.w = srcRect.w;
            dstRect.h = srcRect.h;

        }
        else
        {
            srcRect.x = 0;
            srcRect.y = 0;
            srcRect.w = df.mFrameHeader.mWidth;
            srcRect.h = df.mFrameHeader.mHeight;
        }

        if (mSpriteSheet)
        {
            SDL_BlitSurface(frame.get(), &srcRect, mSpriteSheet.get(), &dstRect);

            mXPos++;
            if (mXPos > mSpritesX)
            {
                mXPos = 0;
                mYPos++;
            }
        }
    }

    void DebugAnimationSpriteSheet::EndFrames()
    {
        if (mSpriteSheet)
        {
            SDLHelpers::SaveSurfaceAsPng((mFileName + "_" + mDataSetName + "_id_" + std::to_string(mId) + ".png").c_str(), mSpriteSheet.get());
        }
    }

    void DebugDumpAnimationFrames(const std::string& fileName, u32 id, IStream& stream, bool bIsPsx, const char* dataSetName)
    {
        AnimSerializer as(stream, bIsPsx);
        DebugAnimationSpriteSheet dass(as, fileName, id, dataSetName);
    }

    AnimSerializer::AnimSerializer(IStream& stream, bool bIsPsx)
        : mIsPsx(bIsPsx), mStream(stream)
    {
        // Read the header
        mStream.Read(mHeader.mMaxW);
        mStream.Read(mHeader.mMaxH);
        mStream.Read(mHeader.mFrameTableOffSet);
        
        // Read the pallete
        const u32 frameStart = ParsePallete();

        // Seek to frame table offset
        mStream.Seek(mHeader.mFrameTableOffSet);

        // Read each animation header
        ParseAnimationSets();

        // Read the info about each frame in each animation header
        ParseFrameInfoHeaders();

        // Get a list of unique image frames
        GatherUniqueFrameOffsets();
      //  mUniqueFrameHeaderOffsets.insert(mHeader.mFrameTableOffSet);

        if (frameStart != 0)
        {
            // TODO: Handle frames as rects into sprite sheet
            mSingleFrameOffset = frameStart;
        }
    }

    u32 AnimSerializer::ParsePallete()
    {
        u32 frameStart = 0;

        mStream.Read(mHeader.mPaltSize);
        mClutOffset = static_cast<u32>(mStream.Pos());
        if (mHeader.mPaltSize == 0)
        {
            // Assume its an Ae file if the palt size is zero, in this case the next u32 is
            // actually the palt size.
            mbIsAoFile = false;
            mClutOffset = static_cast<u32>(mStream.Pos());
            mStream.Read(mHeader.mPaltSize);
        }
        else
        {
            // There is another anim format where all sprite frames are in one
            // pre-made sprite sheet. In this case there are 10 zero bytes for
            // some unused structure.
            // Thus if the next 10 bytes are zeroes then this is "sprite sheet"
            // format anim, else its "normal" AO format anim.
            frameStart = mHeader.mPaltSize;

            std::array<u8, 10> nulls = {};
            mStream.Read(nulls);
            bool allNulls = true;
            for (const auto& b : nulls)
            {
                if (b != 0)
                {
                    allNulls = false;
                    break;
                }
            }
            if (allNulls)
            {
                mStream.Seek(frameStart);

                u32 paltOffset = 0;
                mStream.Read(paltOffset);
                mStream.Seek(paltOffset);
                mClutOffset = static_cast<u32>(mStream.Pos());
                mStream.Read(mHeader.mPaltSize);
            }
            else
            {
                frameStart = 0;
                mStream.Seek(mStream.Pos() - 10);
            }
        }

        // Read the palette
        mPalt.reserve(mHeader.mPaltSize);

        for (auto i = 0u; i < mHeader.mPaltSize; i++)
        {
            u16 tmp = 0;
            mStream.Read(tmp);

            unsigned int oldPixel = tmp;

            // TODO: Only apply to problematic sprites in AO PSX demo
            /*
            if (oldPixel == 0x0400 || oldPixel == 0xe422 || oldPixel == 0x9c00)
            {
            oldPixel = 1 << 15;
            }
            */

            const unsigned short int semiTrans = (((oldPixel) >> 15) & 0xffff);


            // RGB555
            const unsigned short int green16 = ((oldPixel >> 5) & 0x1F);
            const unsigned short int red16 = ((oldPixel >> 0) & 0x1F);
            const unsigned short int blue16 = ((oldPixel >> 10) & 0x1F);

            // TODO: Get colour ramp off real PSX in 16bit mode as they dont 100% map to PC RGB ramp

            const unsigned int green32 = ((green16 * 255) / 31);
            const unsigned int blue32 = ((blue16 * 255) / 31);
            const unsigned int red32 = ((red16 * 255) / 31);


            unsigned int newPixel = (red32 << 24) | (blue32 << 8) | (green32 << 16);
            //unsigned int newPixel = (blue32 << 8) | (green32 << 0) | (red32 << 16);

            if (semiTrans)
            {
                newPixel |= (255 / 2);
            }
            else if (newPixel == 0)
            {
                // Fully transparent
                newPixel |= 0;
            }
            else
            {
                newPixel |= 255;
            }

            mPalt.push_back(newPixel);
        }
        return frameStart;
    }

    void AnimSerializer::ParseAnimationSets()
    {
        // Collect all animation sets
        auto hdr = std::make_unique<AnimationHeader>();
        mStream.Read(hdr->mFps);
        mStream.Read(hdr->mNumFrames);
        mStream.Read(hdr->mLoopStartFrame);
        mStream.Read(hdr->mFlags);
        
        // Read the offsets to each frame info
        hdr->mFrameInfoOffsets.resize(hdr->mNumFrames);
        for (auto& offset : hdr->mFrameInfoOffsets)
        {
            mStream.Read(offset);
        }

        // The first set is usually the last set. Either way when we get to the end of the file from
        // reading the final set we start looking for sets at the end of this sets final info data.
        // Eventually we'll get to the info at iFrameTableOffSet again and stop the recursion.
        // This might be wrong and missing some sets, it might be better to track the position at the end
        // of each set so far and use the one nearest to the EOF.
        if (mStream.AtEnd())
        {
            if (hdr->mFrameInfoOffsets.empty())
            {
                // Handle a very strange case seen in AO ROPES.BAN, we have a set header that says there are
                // zero frames, then we have a single frame offset *before* the start of the animation!
                mStream.Seek(0);
                mStream.Seek(mStream.Size() - 0x14);
                ParseAnimationSets();
            }
            else
            {
                // As we said above move to the last frame info offset
                auto offset = hdr->mFrameInfoOffsets.back();
                mStream.Seek(offset);

                // Now we want to seek to the end of the frame info where we hope the FrameSet data
                // *really* starts (and why we'll end up where we started after parsing down this list)
                auto frmHdr = std::make_unique<FrameInfoHeader>();

                mStream.Read(frmHdr->mFrameHeaderOffset);
                mStream.Read(frmHdr->mMagic);
                // stream.Read(frmHdr->points);
                // stream.Read(frmHdr->triggers);

                // if (frmHdr->points != 0x3)
                {
                    // I think there is always 3 points?
                    //  abort();
                }
                u32 headerDataToSkipSize = 0;
                switch (frmHdr->mMagic)
                {
                    // Always this for AO frames, and almost always this for AE frames
                case 0x3:
                    headerDataToSkipSize = 0x14;
                    break;

                    // Sometimes we  get this for AE!
                case 0x7:
                    headerDataToSkipSize = 0x2C;
                    break;

                    // Sometimes we get this for AE!
                case 0x9:
                    headerDataToSkipSize = 0x2C;
                    break;

                default:
                    headerDataToSkipSize = 0x1C;
                    break;
                }

                mStream.Seek(offset + headerDataToSkipSize);
            }
        }

        mAnimationHeaders.emplace_back(std::move(hdr));

        if (mStream.Pos() != mHeader.mFrameTableOffSet)
        {
            ParseAnimationSets();
        }
    }

    void AnimSerializer::ParseFrameInfoHeaders()
    {
        for (const std::unique_ptr<AnimationHeader>& animationHeader : mAnimationHeaders)
        {
            for (const u32 frameInfoOffset : animationHeader->mFrameInfoOffsets)
            {
                mStream.Seek(frameInfoOffset);

                auto frameInfo = std::make_unique<FrameInfoHeader>();

                // Record where the offsets are for debugging - this gives us an easy way to point all frames
                // to the same image data
                //mUniqueFrameHeaderStreamOffsets.insert(stream.Pos());

                mStream.Read(frameInfo->mFrameHeaderOffset);
                mStream.Read(frameInfo->mMagic);


                mStream.Read(frameInfo->mOffx);
                mStream.Read(frameInfo->mOffy);


                mStream.Read(frameInfo->mTopLeft.x);
                mStream.Read(frameInfo->mTopLeft.y);

          
                mStream.Read(frameInfo->mBottomRight.x);
                mStream.Read(frameInfo->mBottomRight.y);

                animationHeader->mFrameInfos.emplace_back(std::move(frameInfo));
            }
        }
    }

    void AnimSerializer::GatherUniqueFrameOffsets()
    {
        for (const std::unique_ptr<AnimationHeader>& animationHeader : mAnimationHeaders)
        {
            for (const std::unique_ptr<FrameInfoHeader>& frameInfo : animationHeader->mFrameInfos)
            {
                mUniqueFrameHeaderOffsets.insert(frameInfo->mFrameHeaderOffset);
            }
        }
    }

    template<class T>
    std::vector<u8> AnimSerializer::Decompress(AnimSerializer::FrameHeader& header, u32 finalW)
    {
        T decompressor;
        auto decompressedData = decompressor.Decompress(mStream, finalW, header.mWidth, header.mHeight, header.mFrameDataSize);
        return decompressedData;
    }

    u32 AnimSerializer::GetPaltValue(u32 idx)
    {
        // ABEEND.BAN from the AO PSX demo goes out of bounds - probably why the resulting
        // beta image looks quite strange.
        if (idx >= mPalt.size())
        {
            idx = static_cast<unsigned char>(mPalt.size() - 1);
        }
        
        return mPalt[idx];
    }

    SDL_SurfacePtr AnimSerializer::ApplyPalleteToFrame(const FrameHeader& header, u32 realWidth, const std::vector<u8>& decompressedData, std::vector<u32>& pixels)
    {
        // Apply the pallete
        if (header.mColourDepth == 8)
        {
            // TODO: Could recycle buffer
            pixels.reserve(pixels.size());
            for (auto v : decompressedData)
            {
                pixels.push_back(GetPaltValue(v));
            }
            // Create an SDL surface
            const auto red_mask = 0xff000000;
            const auto green_mask = 0x00ff0000;
            const auto blue_mask = 0x0000ff00;
            const auto alpha_mask = 0x000000ff;
            SDL_SurfacePtr surface(SDL_CreateRGBSurfaceFrom(pixels.data(), header.mWidth, header.mHeight, 32, realWidth*sizeof(u32), red_mask, green_mask, blue_mask, alpha_mask));

            return surface;
        }
        else if (header.mColourDepth == 4)
        {
#define HI_NIBBLE(b) (((b) >> 4) & 0x0F)
#define LO_NIBBLE(b) ((b) & 0x0F)

            pixels.reserve(decompressedData.size() * 2);
            for (auto v : decompressedData)
            {
                pixels.push_back(GetPaltValue(LO_NIBBLE(v)));
                pixels.push_back(GetPaltValue(HI_NIBBLE(v)));
            }

            // Create an SDL surface
            const auto red_mask = 0xff000000;
            const auto green_mask = 0x00ff0000;
            const auto blue_mask = 0x0000ff00;
            const auto alpha_mask = 0x000000ff;
            SDL_SurfacePtr surface(SDL_CreateRGBSurfaceFrom(pixels.data(), header.mWidth, header.mHeight, 32, realWidth*sizeof(u32), red_mask, green_mask, blue_mask, alpha_mask));

            return surface;
        }
        else
        {
            abort();
        }
    }

    AnimSerializer::DecodedFrame AnimSerializer::ReadAndDecompressFrame(u32 frameOffset)
    {
        DecodedFrame ret;

        if (mSingleFrameOffset > 0)
        {
            // There is only one frame here.. can't seek anywhere else!
            mStream.Seek(mSingleFrameOffset);
        }
        else
        {
            mStream.Seek(frameOffset);
        }

        FrameHeader frameHeader;
        mStream.Read(frameHeader.mClutOffset);


        mStream.Read(frameHeader.mWidth);
        mStream.Read(frameHeader.mHeight);
        mStream.Read(frameHeader.mColourDepth);
        mStream.Read(frameHeader.mCompressionType);
        mStream.Read(frameHeader.mFrameDataSize);

        u32 nTextureWidth = 0;
        u32 actualWidth = 0;
        if (frameHeader.mColourDepth == 8)
        {
            nTextureWidth = ((frameHeader.mWidth + 3) / 2) & ~1;
            actualWidth = nTextureWidth * 2;
        }
        else if (frameHeader.mColourDepth == 16)
        {
            nTextureWidth = ((frameHeader.mWidth + 1)) & ~1;
            actualWidth = nTextureWidth;
        }
        else if (frameHeader.mColourDepth == 4)
        {
            nTextureWidth = ((frameHeader.mWidth + 7) / 4)&~1;
            actualWidth = nTextureWidth * 4;
        }
        else
        {
            abort();
            //LOG_ERROR("Bad depth");
            //return std::vector<u8>();
        }
        ret.mFixedWidth = actualWidth;

        // TODO: Sometimes the offset isn't the same, need to check why, multi cluts?
       // assert(frameHeader.mClutOffset == mClutPos);

        //LOG_INFO("TYPE: " << (int)frameHeader.mCompressionType << " DEPTH " << (int)frameHeader.mColourDepth);

        switch (frameHeader.mCompressionType)
        {
        case 0:
            // Used in AE and AO (seems to mean "no compression"?)
            {

                // The frame size field isn't used in this case, its actually part of the frame pixel data
                mStream.Seek(mStream.Pos() - 4);

                if (frameHeader.mColourDepth == 4)
                {
                    ret.mPixelData.resize((actualWidth*frameHeader.mHeight) / 2);
                }
                else if (frameHeader.mColourDepth == 8)
                {
                    ret.mPixelData.resize(actualWidth*frameHeader.mHeight);
                }
                else
                {
                    abort();
                }

                if (!ret.mPixelData.empty())
                {
                    mStream.Read(ret.mPixelData);
                }
                else
                {
                    LOG_WARNING("Empty frame data");
                }
            }
            break;

        case 1:
            // In AE and AO but never used. Both same algorithm, 0x0040A610 in AE
            abort();
            break;

        case 2:
           // In AE but never used, used for AO, same algorithm, 0x0040AA50 in AE
            ret.mPixelData = Decompress<CompressionType2>(frameHeader, actualWidth);
            break;

        case 3:
            if (frameHeader.mClutOffset == 0x8)
            {
                // The size is the header seems to be half the size of the calculated frameDataSize, give or take 3 bytes
                ret.mPixelData = Decompress<CompressionType3>(frameHeader, actualWidth );
            }
            else
            {
                ret.mPixelData = Decompress<CompressionType3Ae>(frameHeader, actualWidth);
            }
            break;

        case 4:
        case 5:
            // Both AO and AE
            ret.mPixelData = Decompress<CompressionType4Or5>(frameHeader, actualWidth);
            break;

        // AO cases end at 5
        case 6:
            if (mIsPsx)
            {
                // Actually type 7 in AE PC

                // This clips off extra "bad" pixels that sometimes get added
                frameHeader.mHeight -= 1;
                ret.mPixelData = Decompress<CompressionType6or7AePsx<8>>(frameHeader, actualWidth);
            }
            else
            {
                ret.mPixelData = Decompress<CompressionType6Ae>(frameHeader, actualWidth);
            }
            break;
            
        case 7:
           // Actually type 8 in AE PC
            if (mIsPsx)
            {
                // This clips off extra "bad" pixels that sometimes get added
                frameHeader.mHeight -= 1;
                ret.mPixelData = Decompress<CompressionType6or7AePsx<6>>(frameHeader, actualWidth);
            }
            else
            {
                // TODO: ABEINTRO.BAN in AE PC has this? Check correctness

                // This clips off extra "bad" pixels that sometimes get added
                frameHeader.mHeight -= 1;
                ret.mPixelData = Decompress<CompressionType6or7AePsx<8>>(frameHeader, actualWidth);
            }
            break;

        case 8:
            // AE PC never used - same as type 7 in AE PSX
            abort();
            break;
        }

        ret.mFrameHeader = frameHeader;

        return ret;
    }
}
