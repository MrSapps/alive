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
#include "lodepng/lodepng.h"
#include <assert.h>
#include <fstream>
#include <array>

namespace Oddlib
{
    AnimationSet::AnimationSet(AnimSerializer& /*as*/)
    {

    }

    std::unique_ptr<AnimationSet> LoadAnimations(const std::string& /*fileName*/, Uint32 /*id*/, IStream& stream, bool bIsPsx, const char* /*dataSetName*/)
    {
        AnimSerializer as(stream, bIsPsx);
        return nullptr;
    }

    DebugAnimationSpriteSheet::DebugAnimationSpriteSheet(AnimSerializer& as, const std::string& fileName, Uint32 id, const char* dataSetName)
        : mFileName(fileName), mDataSetName(dataSetName), mId(id)
    {
        DebugDecodeAllFrames(as);
    }


    void DebugAnimationSpriteSheet::DebugDecodeAllFrames(AnimSerializer& as)
    {
        auto endIt = as.UniqueFrames().end();
        std::advance(endIt, -1);

        BeginFrames(as.MaxW(), as.MaxH(), static_cast<int>(as.UniqueFrames().size()));

        for (auto it = as.UniqueFrames().begin(); it != endIt; it++)
        {
            // In most cases data size isn't used, instead the frame size from the header
            // of the frame is used instead.
            auto frameData = as.ReadAndDecompressFrame(*it, DataSize(as, it));
            AddFrame(as, frameData);
        }

        EndFrames();
    }

    Uint32 DebugAnimationSpriteSheet::DataSize(AnimSerializer& as, std::set<Uint32>::iterator it)
    {
        auto nextIt = it;
        nextIt++;
        if (nextIt == as.UniqueFrames().end())
        {
            // TODO Since we added mHeader.mFrameTableOffSet to mUniqueFrameHeaderOffsets
            // can't happen?
            //return mHeader.mFrameTableOffSet - *it;
            abort();
        }
        else
        {
            return *nextIt - *it;
        }
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

    void DebugAnimationSpriteSheet::AddFrame(AnimSerializer& as, AnimSerializer::DecodedFrame& df)
    {
        int xpos = mXPos * as.MaxW();
        int ypos = mYPos * as.MaxH();

        std::vector<Uint32> pixels;
        auto frame = as.ApplyPalleteToFrame(df.mFrameHeader, df.mFixedWidth, df.mPixelData, pixels);


        SDL_Rect dstRect;
        dstRect.x = xpos;
        dstRect.y = ypos;
        dstRect.w = df.mFrameHeader.mWidth;
        dstRect.h = df.mFrameHeader.mHeight;

        SDL_Rect srcRect;
        srcRect.x = 0;
        srcRect.y = 0;
        srcRect.w = df.mFrameHeader.mWidth;
        srcRect.h = df.mFrameHeader.mHeight;

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
            lodepng::State state = {};

            // input color type
            state.info_raw.colortype = LCT_RGBA;
            state.info_raw.bitdepth = 8;

            // output color type
            state.info_png.color.colortype = LCT_RGBA;
            state.info_png.color.bitdepth = 8;
            state.encoder.auto_convert = 0;

            // encode to PNG
            std::vector<unsigned char> out;
            lodepng::encode(out, (const unsigned char*)mSpriteSheet->pixels, mSpriteSheet->w, mSpriteSheet->h, state);

            // write out PNG buffer
            std::ofstream fileStream;
            fileStream.open((mFileName + "_" + mDataSetName + "_id_" + std::to_string(mId) + ".png").c_str(), std::ios::binary);
            if (!fileStream.is_open())
            {
                throw Exception("Can't open output file");
            }

            fileStream.write(reinterpret_cast<const char*>(out.data()), out.size());
        }
    }

    void DebugDumpAnimationFrames(const std::string& fileName, Uint32 id, IStream& stream, bool bIsPsx, const char* dataSetName)
    {
        AnimSerializer as(stream, bIsPsx);
        DebugAnimationSpriteSheet dass(as, fileName, id, dataSetName);
    }

    AnimSerializer::AnimSerializer(IStream& stream, bool bIsPsx)
        : mIsPsx(bIsPsx), mStream(stream)
    {
        // Read the header
        mStream.ReadUInt16(mHeader.mMaxW);
        mStream.ReadUInt16(mHeader.mMaxH);
        mStream.ReadUInt32(mHeader.mFrameTableOffSet);
        
        // Read the pallete
        const Uint32 frameStart = ParsePallete();

        // Seek to frame table offset
        mStream.Seek(mHeader.mFrameTableOffSet);

        // Read each animation header
        ParseAnimationSets();

        // Read the info about each frame in each animation header
        ParseFrameInfoHeaders();

        // Get a list of unique image frames
        GatherUniqueFrameOffsets();
        mUniqueFrameHeaderOffsets.insert(mHeader.mFrameTableOffSet);

        if (frameStart != 0)
        {
            // TODO: Handle frames as rects into sprite sheet
            mSingleFrameOffset = frameStart;
        }
    }

    Uint32 AnimSerializer::ParsePallete()
    {
        Uint32 frameStart = 0;

        mStream.ReadUInt32(mHeader.mPaltSize);
        mClutOffset = static_cast<Uint32>(mStream.Pos());
        if (mHeader.mPaltSize == 0)
        {
            // Assume its an Ae file if the palt size is zero, in this case the next Uint32 is
            // actually the palt size.
            mbIsAoFile = false;
            mClutOffset = static_cast<Uint32>(mStream.Pos());
            mStream.ReadUInt32(mHeader.mPaltSize);
        }
        else
        {
            // There is another anim format where all sprite frames are in one
            // pre-made sprite sheet. In this case there are 10 zero bytes for
            // some unused structure.
            // Thus if the next 10 bytes are zeroes then this is "sprite sheet"
            // format anim, else its "normal" AO format anim.
            frameStart = mHeader.mPaltSize;

            std::array<Uint8, 10> nulls = {};
            mStream.ReadBytes(nulls.data(), nulls.size());
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

                Uint32 paltOffset = 0;
                mStream.ReadUInt32(paltOffset);
                mStream.Seek(paltOffset);
                mClutOffset = static_cast<Uint32>(mStream.Pos());
                mStream.ReadUInt32(mHeader.mPaltSize);
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
            Uint16 tmp = 0;
            mStream.ReadUInt16(tmp);

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
                // Should be 50% but slightly less looks better
                newPixel |= 180;// (255 / 2);
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
        mStream.ReadUInt16(hdr->mFps);
        mStream.ReadUInt16(hdr->mNumFrames);
        mStream.ReadUInt16(hdr->mLoopStartFrame);
        mStream.ReadUInt16(hdr->mFlags);
        
        // Read the offsets to each frame info
        hdr->mFrameInfoOffsets.resize(hdr->mNumFrames);
        for (auto& offset : hdr->mFrameInfoOffsets)
        {
            mStream.ReadUInt32(offset);
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

                mStream.ReadUInt32(frmHdr->mFrameHeaderOffset);
                mStream.ReadUInt32(frmHdr->mMagic);
                // stream.ReadUInt16(frmHdr->points);
                // stream.ReadUInt16(frmHdr->triggers);

                // if (frmHdr->points != 0x3)
                {
                    // I think there is always 3 points?
                    //  abort();
                }
                Uint32 headerDataToSkipSize = 0;
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
            for (const Uint32 frameInfoOffset : animationHeader->mFrameInfoOffsets)
            {
                mStream.Seek(frameInfoOffset);

                auto frameInfo = std::make_unique<FrameInfoHeader>();

                // Record where the offsets are for debugging - this gives us an easy way to point all frames
                // to the same image data
                //mUniqueFrameHeaderStreamOffsets.insert(stream.Pos());

                mStream.ReadUInt32(frameInfo->mFrameHeaderOffset);
                mStream.ReadUInt32(frameInfo->mMagic);

                mStream.ReadSInt16(frameInfo->mColx);
                mStream.ReadSInt16(frameInfo->mColy);

                mStream.ReadSInt16(frameInfo->mColw);
                mStream.ReadSInt16(frameInfo->mColh);

                mStream.ReadSInt16(frameInfo->mOffx);
                mStream.ReadSInt16(frameInfo->mOffy);

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
    std::vector<Uint8> AnimSerializer::Decompress(AnimSerializer::FrameHeader& /*header*/, Uint32 finalW, Uint32 w, Uint32 h, Uint32 dataSize)
    {
        T decompressor;
        auto decompressedData = decompressor.Decompress(mStream, finalW, w, h, dataSize);
        //AddFrame(header, finalW, decompressedData);
        return decompressedData;
    }

    Uint32 AnimSerializer::GetPaltValue(Uint32 idx)
    {
        // ABEEND.BAN from the AO PSX demo goes out of bounds - probably why the resulting
        // beta image looks quite strange.
        if (idx >= mPalt.size())
        {
            idx = static_cast<unsigned char>(mPalt.size() - 1);
        }
        
        return mPalt[idx];
    }

    SDL_SurfacePtr AnimSerializer::ApplyPalleteToFrame(FrameHeader& header, Uint32 realWidth, const std::vector<Uint8>& decompressedData, std::vector<Uint32>& pixels)
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
            SDL_SurfacePtr surface(SDL_CreateRGBSurfaceFrom(pixels.data(), header.mWidth, header.mHeight, 32, realWidth*sizeof(Uint32), red_mask, green_mask, blue_mask, alpha_mask));

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
            SDL_SurfacePtr surface(SDL_CreateRGBSurfaceFrom(pixels.data(), header.mWidth, header.mHeight, 32, realWidth*sizeof(Uint32), red_mask, green_mask, blue_mask, alpha_mask));

            return surface;
        }
        else
        {
            abort();
        }
    }

    AnimSerializer::DecodedFrame AnimSerializer::ReadAndDecompressFrame(Uint32 frameOffset, Uint32 frameDataSize)
    {
        DecodedFrame ret;

        mStream.Seek(frameOffset);

        FrameHeader frameHeader;
        mStream.ReadUInt32(frameHeader.mClutOffset);


        mStream.ReadUInt8(frameHeader.mWidth);
        mStream.ReadUInt8(frameHeader.mHeight);
        mStream.ReadUInt8(frameHeader.mColourDepth);
        mStream.ReadUInt8(frameHeader.mCompressionType);
        mStream.ReadUInt32(frameHeader.mFrameDataSize);

        Uint32 nTextureWidth = 0;
        Uint32 actualWidth = 0;
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
            //return std::vector<Uint8>();
        }
        ret.mFixedWidth = actualWidth;
        ret.mFrameHeader = frameHeader;

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
                    mStream.ReadBytes(ret.mPixelData.data(), ret.mPixelData.size());
                    //AddFrame(frameHeader, actualWidth, data);
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
            ret.mPixelData = Decompress<CompressionType2>(frameHeader, actualWidth, frameHeader.mWidth, frameHeader.mHeight, (frameHeader.mClutOffset == 0x8 || mIsPsx) ? frameHeader.mFrameDataSize : frameDataSize);
            break;

        case 3:
            if (frameHeader.mClutOffset == 0x8)
            {
                // The size is the header seems to be half the size of the calculated frameDataSize, give or take 3 bytes
                ret.mPixelData = Decompress<CompressionType3>(frameHeader, actualWidth, frameHeader.mWidth, frameHeader.mHeight, frameHeader.mFrameDataSize);
            }
            else
            {
                ret.mPixelData = Decompress<CompressionType3Ae>(frameHeader, actualWidth, frameHeader.mWidth, frameHeader.mHeight, frameHeader.mClutOffset == 0x8 ? frameHeader.mFrameDataSize : frameDataSize);
            }
            break;

        case 4:
        case 5:
            // Both AO and AE
            ret.mPixelData = Decompress<CompressionType4Or5>(frameHeader, actualWidth, frameHeader.mWidth, frameHeader.mHeight, frameHeader.mClutOffset == 0x8 ? frameHeader.mFrameDataSize : frameDataSize);
            break;

        // AO cases end at 5
        case 6:
            if (mIsPsx)
            {
                // Actually type 7 in AE PC

                // This clips off extra "bad" pixels that sometimes get added
                frameHeader.mHeight -= 1;
                ret.mPixelData = Decompress<CompressionType6or7AePsx<8>>(frameHeader, actualWidth, frameHeader.mWidth, frameHeader.mHeight, frameHeader.mFrameDataSize);
            }
            else
            {
                ret.mPixelData = Decompress<CompressionType6Ae>(frameHeader, actualWidth, frameHeader.mWidth, frameHeader.mHeight, frameHeader.mFrameDataSize);
            }
            break;
            
        case 7:
           // Actually type 8 in AE PC
            if (mIsPsx)
            {
                // This clips off extra "bad" pixels that sometimes get added
                frameHeader.mHeight -= 1;
                ret.mPixelData = Decompress<CompressionType6or7AePsx<6>>(frameHeader, actualWidth, frameHeader.mWidth, frameHeader.mHeight, frameHeader.mFrameDataSize);
            }
            else
            {
                // TODO: ABEINTRO.BAN in AE PC has this? Check correctness

                // This clips off extra "bad" pixels that sometimes get added
                frameHeader.mHeight -= 1;
                ret.mPixelData = Decompress<CompressionType6or7AePsx<8>>(frameHeader, actualWidth, frameHeader.mWidth, frameHeader.mHeight, frameHeader.mFrameDataSize);
            }
            break;

        case 8:
            // AE PC never used - same as type 7 in AE PSX
            abort();
            break;
        }

        return ret;
    }
}
