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
#include <fstream>
#include <array>

namespace Oddlib
{

    AnimSerializer::AnimSerializer(const std::string& fileName, Uint32 id, IStream& stream, bool bIsPsx, const char* dataSetName)
        : mFileName(fileName), mId(id), mIsPsx(bIsPsx), mDataSetName(dataSetName)
    {
        
        //stream.BinaryDump(fileName + "_" + std::to_string(mId));

        // Read the header
        stream.ReadUInt16(mHeader.mMaxW);
        stream.ReadUInt16(mHeader.mMaxH);
        stream.ReadUInt32(mHeader.mFrameTableOffSet);

         Uint32 frameStart = 0;

        stream.ReadUInt32(mHeader.mPaltSize);
        if (mHeader.mPaltSize == 0)
        {
            // Assume its an Ae file if the palt size is zero, in this case the next Uint32 is
            // actually the palt size.
            mbIsAoFile = false;
            stream.ReadUInt32(mHeader.mPaltSize);
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
            stream.ReadBytes(nulls.data(), nulls.size());
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
                stream.Seek(frameStart);

                Uint32 paltOffset = 0;
                stream.ReadUInt32(paltOffset);
                stream.Seek(paltOffset);
                stream.ReadUInt32(mHeader.mPaltSize);
            }
            else
            {
                frameStart = 0;
                stream.Seek(stream.Pos() - 10);
            }
        }

        // Read the palette
        mPalt.reserve(mHeader.mPaltSize);
        for (auto i = 0u; i < mHeader.mPaltSize; i++)
        {
            Uint16 tmp = 0;
            stream.ReadUInt16(tmp);


            unsigned int oldPixel = tmp;

            // RGB555
            unsigned short int green = (((oldPixel >> 5) & 0x1F) << 6);
            unsigned short int blue = (((oldPixel >> 10) & 0x1F) << 0);
            unsigned short int red = (((oldPixel >> 0) & 0x1F) << 11);

            oldPixel = (static_cast<unsigned short int> (oldPixel) >> 15) & 0xffff; // Checking transparent bit?
            if (oldPixel)
            {
                //LOG_INFO("Transparent");
            }

            unsigned short int newPixel = red | blue | green;


            mPalt.push_back(newPixel);
        }

        // Seek to frame table offset
        stream.Seek(mHeader.mFrameTableOffSet);

        ParseAnimationSets(stream);
        ParseFrameInfoHeaders(stream);
        GatherUniqueFrameOffsets();
        mUniqueFrameHeaderOffsets.insert(mHeader.mFrameTableOffSet);
        // TODO: Handle frames as rects into sprite sheet
        if (frameStart != 0)
        {
            mUniqueFrameHeaderOffsets.clear();
            mUniqueFrameHeaderOffsets.insert(frameStart);
            mUniqueFrameHeaderOffsets.insert(mHeader.mFrameTableOffSet);
        }

        DebugDecodeAllFrames(stream);
    }

    void AnimSerializer::ParseAnimationSets(IStream& stream)
    {
        // Collect all animation sets
        auto hdr = std::make_unique<AnimationHeader>();
        stream.ReadUInt16(hdr->mFps);
        stream.ReadUInt16(hdr->mNumFrames);
        stream.ReadUInt16(hdr->mLoopStartFrame);
        stream.ReadUInt16(hdr->mFlags);
        
        // Read the offsets to each frame info
        hdr->mFrameInfoOffsets.resize(hdr->mNumFrames);
        for (auto& offset : hdr->mFrameInfoOffsets)
        {
            stream.ReadUInt32(offset);
        }

        // The first set is usually the last set. Either way when we get to the end of the file from
        // reading the final set we start looking for sets at the end of this sets final info data.
        // Eventually we'll get to the info at iFrameTableOffSet again and stop the recursion.
        // This might be wrong and missing some sets, it might be better to track the position at the end
        // of each set so far and use the one nearest to the EOF.
        if (stream.AtEnd())
        {
            if (hdr->mFrameInfoOffsets.empty())
            {
                // Handle a very strange case seen in AO ROPES.BAN, we have a set header that says there are
                // zero frames, then we have a single frame offset *before* the start of the animation!
                stream.Seek(0);
                stream.Seek(stream.Size() - 0x14);
                ParseAnimationSets(stream);
            }
            else
            {
                // As we said above move to the last frame info offset
                auto offset = hdr->mFrameInfoOffsets.back();
                stream.Seek(offset);

                // Now we want to seek to the end of the frame info where we hope the FrameSet data
                // *really* starts (and why we'll end up where we started after parsing down this list)
                auto frmHdr = std::make_unique<FrameInfoHeader>();

                stream.ReadUInt32(frmHdr->mFrameHeaderOffset);
                stream.ReadUInt32(frmHdr->mMagic);
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

                stream.Seek(offset + headerDataToSkipSize);
            }
        }

        mAnimationHeaders.emplace_back(std::move(hdr));

        if (stream.Pos() != mHeader.mFrameTableOffSet)
        {
            ParseAnimationSets(stream);
        }
    }

    void AnimSerializer::ParseFrameInfoHeaders(IStream& stream)
    {
        for (const std::unique_ptr<AnimationHeader>& animationHeader : mAnimationHeaders)
        {
            for (const Uint32 frameInfoOffset : animationHeader->mFrameInfoOffsets)
            {
                stream.Seek(frameInfoOffset);

                auto frameInfo = std::make_unique<FrameInfoHeader>();

                stream.ReadUInt32(frameInfo->mFrameHeaderOffset);
                stream.ReadUInt32(frameInfo->mMagic);

                stream.ReadSInt16(frameInfo->mColx);
                stream.ReadSInt16(frameInfo->mColy);

                stream.ReadSInt16(frameInfo->mColw);
                stream.ReadSInt16(frameInfo->mColh);

                stream.ReadSInt16(frameInfo->mOffx);
                stream.ReadSInt16(frameInfo->mOffy);

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

    Uint32 AnimSerializer::DataSize(std::set<Uint32>::iterator it)
    {
        auto nextIt = it;
        nextIt++;
        if (nextIt == mUniqueFrameHeaderOffsets.end())
        {
            // TODO Since we added mHeader.mFrameTableOffSet to mUniqueFrameHeaderOffsets
            // can't happen?
            return mHeader.mFrameTableOffSet - *it;
        }
        else
        {
            return *nextIt - *it;
        }
    }

    void AnimSerializer::DebugDecodeAllFrames(IStream& stream)
    {
        /*
        auto endIt = mUniqueFrameHeaderOffsets.end();
        std::advance(endIt, -1);

        for (auto it = mUniqueFrameHeaderOffsets.begin(); it != endIt; it++)
        {
            DecodeFrame(stream, *it, DataSize(it));
        }
        */

        auto endIt = mUniqueFrameHeaderOffsets.end();
        std::advance(endIt, -1);

        BeginFrames(mHeader.mMaxW, mHeader.mMaxH, static_cast<int>(mUniqueFrameHeaderOffsets.size()));
        for (auto it = mUniqueFrameHeaderOffsets.begin(); it != endIt; it++)
        {
            DecodeFrame(stream, *it, DataSize(it));
        }
        EndFrames();
    }

    template<class T>
    std::vector<Uint8> AnimSerializer::Decompress(AnimSerializer::FrameHeader& header, IStream& stream, Uint32 finalW, Uint32 w, Uint32 h, Uint32 dataSize)
    {
        T decompressor;
        auto decompressedData = decompressor.Decompress(stream, finalW, w, h, dataSize);
        AddFrame(header, finalW, decompressedData);
        return decompressedData;
    }

    void AnimSerializer::BeginFrames(int w, int h, int count)
    {
        // BUG: Sprite sheet types are truncated to max frame size

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

        const auto red_mask = 0xF800;
        const auto green_mask = 0x7E0;
        const auto blue_mask = 0x1F;
        mSpriteSheet.reset(SDL_CreateRGBSurface(0, mSpritesX*w, mSpritesY*h, 16, red_mask, green_mask, blue_mask, 0));
    }

    void AnimSerializer::AddFrame(FrameHeader& header, Uint32 realWidth, const std::vector<Uint8>& decompressedData)
    {

        //DebugSaveFrame(header, realWidth, decompressedData);

        int xpos = mXPos * mHeader.mMaxW;
        int ypos = mYPos * mHeader.mMaxH;

        std::vector<Uint16> pixels;
        auto frame = MakeFrame(header, realWidth, decompressedData, pixels);


        SDL_Rect dstRect;
        dstRect.x = xpos;
        dstRect.y = ypos;
        dstRect.w = frame->w;
        dstRect.h = frame->h;
        SDL_BlitSurface(frame.get(), NULL, mSpriteSheet.get(), &dstRect);

        mXPos++;
        if (mXPos > mSpritesX)
        {
            mXPos = 0;
            mYPos++;
        }
    }

    void AnimSerializer::EndFrames()
    {
        // Save surface to disk
        SDL_SaveBMP(mSpriteSheet.get(), (mFileName + "_" + mDataSetName + "_id_" + std::to_string(mId) + ".bmp").c_str());
    }

    SDL_SurfacePtr AnimSerializer::MakeFrame(FrameHeader& header, Uint32 realWidth, const std::vector<Uint8>& decompressedData, std::vector<Uint16>& pixels)
    {
        // Apply the pallete
        if (header.mColourDepth == 8)
        {
            // TODO: Could recycle buffer
            pixels.reserve(pixels.size());
            for (auto v : decompressedData)
            {
                // ABEEND.BAN from the AO PSX demo goes out of bounds - probably why the resulting
                // beta image looks quite strange.
                if (v >= mPalt.size())
                {
                    v = static_cast<unsigned char>(mPalt.size() - 1);
                }
                pixels.push_back(mPalt[v]);
            }
            // Create an SDL surface
            const auto red_mask = 0xF800;
            const auto green_mask = 0x7E0;
            const auto blue_mask = 0x1F;
            SDL_SurfacePtr surface(SDL_CreateRGBSurfaceFrom(pixels.data(), realWidth, header.mHeight, 16, realWidth*sizeof(Uint16), red_mask, green_mask, blue_mask, 0));

            return surface;
        }
        else if (header.mColourDepth == 4)
        {
#define HI_NIBBLE(b) (((b) >> 4) & 0x0F)
#define LO_NIBBLE(b) ((b) & 0x0F)

            pixels.reserve(decompressedData.size() * 2);
            for (auto v : decompressedData)
            {
                pixels.push_back(mPalt[LO_NIBBLE(v)]);
                pixels.push_back(mPalt[HI_NIBBLE(v)]);
            }

            // Create an SDL surface
            const auto red_mask = 0xF800;
            const auto green_mask = 0x7E0;
            const auto blue_mask = 0x1F;
            SDL_SurfacePtr surface(SDL_CreateRGBSurfaceFrom(pixels.data(), realWidth, header.mHeight, 16, realWidth*sizeof(Uint16), red_mask, green_mask, blue_mask, 0));

            return surface;
        }
        else
        {
            abort();
        }
    }

    void AnimSerializer::DebugSaveFrame(AnimSerializer::FrameHeader& header, Uint32 realWidth, const std::vector<Uint8>& decompressedData)
    {
        std::vector<Uint16> pixels;
        auto surface = MakeFrame(header, realWidth, decompressedData, pixels);

        // Save surface to disk
        SDL_SaveBMP(surface.get(), (mFileName + "_" + mDataSetName + "_id_" + std::to_string(mId) + ".bmp").c_str());
    }

    std::vector<Uint8> AnimSerializer::DecodeFrame(IStream& stream, Uint32 frameOffset, Uint32 frameDataSize)
    {
        stream.Seek(frameOffset);

        FrameHeader frameHeader;
        stream.ReadUInt32(frameHeader.mClutOffset);

        // TODO: Validate clut offset is valid

        stream.ReadUInt8(frameHeader.mWidth);
        stream.ReadUInt8(frameHeader.mHeight);
        stream.ReadUInt8(frameHeader.mColourDepth);
        stream.ReadUInt8(frameHeader.mCompressionType);
        stream.ReadUInt32(frameHeader.mFrameDataSize);

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

        switch (frameHeader.mCompressionType)
        {
        case 0:
            // Used in AE and AO (seems to mean "no compression"?)
            {
                std::vector<Uint8> data;

                // The frame size field isn't used in this case, its actually part of the frame pixel data
                stream.Seek(stream.Pos() - 4);

                if (frameHeader.mColourDepth == 4)
                {
                    data.resize((actualWidth*frameHeader.mHeight) / 2);
                }
                else if (frameHeader.mColourDepth == 8)
                {
                    data.resize(actualWidth*frameHeader.mHeight);
                }
                else
                {
                    abort();
                }

                if (!data.empty())
                {
                    stream.ReadBytes(data.data(), data.size());
                    AddFrame(frameHeader, actualWidth, data);
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
            Decompress<CompressionType2>(frameHeader, stream, actualWidth, frameHeader.mWidth, frameHeader.mHeight, frameHeader.mClutOffset == 0x8 ? frameHeader.mFrameDataSize : frameDataSize);
            break;

        case 3:
            if (frameHeader.mClutOffset == 0x8)
            {
                // The size is the header seems to be half the size of the calculated frameDataSize, give or take 3 bytes
                Decompress<CompressionType3>(frameHeader, stream, actualWidth, frameHeader.mWidth, frameHeader.mHeight, frameHeader.mFrameDataSize);
            }
            else
            {
                Decompress<CompressionType3Ae>(frameHeader, stream, actualWidth, frameHeader.mWidth, frameHeader.mHeight, frameHeader.mClutOffset == 0x8 ? frameHeader.mFrameDataSize : frameDataSize);
            }
            break;

        case 4:
        case 5:
            // Both AO and AE
            Decompress<CompressionType4Or5>(frameHeader, stream, actualWidth, frameHeader.mWidth, frameHeader.mHeight, frameHeader.mClutOffset == 0x8 ? frameHeader.mFrameDataSize : frameDataSize);
            break;

        // AO cases end at 5
        case 6:
            if (mIsPsx)
            {
                // Actually type 7 in AE PC
                Decompress<CompressionType6or7AePsx<8>>(frameHeader, stream, actualWidth, frameHeader.mHeight, frameHeader.mHeight, frameHeader.mFrameDataSize);
            }
            else
            {
                Decompress<CompressionType6Ae>(frameHeader, stream, actualWidth, frameHeader.mWidth, frameHeader.mHeight, frameHeader.mFrameDataSize);
            }
            break;
            
        case 7:
           // Actually type 8 in AE PC
            if (mIsPsx)
            {
                Decompress<CompressionType6or7AePsx<6>>(frameHeader, stream, actualWidth, frameHeader.mHeight, frameHeader.mHeight, frameHeader.mFrameDataSize);
            }
            else
            {
                // TODO: ABEINTRO.BAN in AE PC has this? Check correctness
                Decompress<CompressionType6or7AePsx<8>>(frameHeader, stream, actualWidth, frameHeader.mHeight, frameHeader.mHeight, frameHeader.mFrameDataSize);
            }
            break;

        case 8:
            // AE PC never used - same as type 7 in AE PSX
            abort();
            break;
        }

        return std::vector<Uint8>();
    }

    // ==================================================

    /*static*/ std::vector<std::unique_ptr<Animation>> AnimationFactory::Create(Oddlib::LvlArchive& archive, const std::string& fileName, Uint32 resourceId, bool bIsxPsx)
    {
        std::vector < std::unique_ptr<Animation> > r;

        Stream stream(archive.FileByName(fileName)->ChunkById(resourceId)->ReadData());
        AnimSerializer anim(fileName, resourceId, stream, bIsxPsx, "unknown");

        return r;
    }

}
