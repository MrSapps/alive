#include "oddlib/anim.hpp"
#include "oddlib/lvlarchive.hpp"
#include "oddlib/stream.hpp"
#include <assert.h>

namespace Oddlib
{
    AnimSerializer::AnimSerializer(IStream& stream)
    {
        // Read the header
        stream.ReadUInt16(h.mMaxW);
        stream.ReadUInt16(h.mMaxH);
        stream.ReadUInt32(h.mFrameTableOffSet);
        stream.ReadUInt32(h.mPaltSize);
        if (h.mPaltSize == 0)
        {
            // Assume its an Ae file if the palt size is zero, in this case the next Uint32 is
            // actually the palt size.
            mbIsAoFile = false;
            stream.ReadUInt32(h.mPaltSize);
        }

        // Read the palette
        for (auto i = 0u; i < h.mPaltSize; i++)
        {
            // TODO Palt obj
            Uint16 tmp = 0;
            stream.ReadUInt16(tmp);
        }

        // Seek to frame table offset
        stream.Seek(h.mFrameTableOffSet);

        ParseAnimationSets(stream);
        ParseFrameInfoHeaders(stream);
        GatherUniqueFrameOffsets();
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

        if (stream.Pos() != h.mFrameTableOffSet)
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

    void AnimSerializer::DebugDecodeAllFrames(IStream& stream)
    {
        for (const Uint32 frameHeaderOffset : mUniqueFrameHeaderOffsets)
        {
            DecodeFrame(stream, frameHeaderOffset);
        }
    }

    std::vector<Uint8> AnimSerializer::DecodeFrame(IStream& stream, Uint32 frameOffset)
    {
        stream.Seek(frameOffset);

        FrameHeader frameHeader;
        stream.ReadUInt32(frameHeader.mMagic);
        stream.ReadUInt8(frameHeader.mWidth);
        stream.ReadUInt8(frameHeader.mHeight);
        stream.ReadUInt8(frameHeader.mColourDepth);
        stream.ReadUInt8(frameHeader.mCompressionType);
        stream.ReadUInt32(frameHeader.mFrameDataSize);

        Uint32 nTextureWidth = 0;
        if (frameHeader.mColourDepth == 8)
        {
            nTextureWidth = ((frameHeader.mWidth + 3) / 2) & ~1;
        }
        else if (frameHeader.mColourDepth == 16)
        {
            nTextureWidth = ((frameHeader.mWidth + 1)) & ~1;
        }
        else
        {
            // 4?
            assert(frameHeader.mColourDepth == 4);
            nTextureWidth = ((frameHeader.mWidth + 7) / 4)&~1;
        }

        // TODO: Decompressors

        switch (frameHeader.mCompressionType)
        {
        case 0:
            // Used in AE and AO (seems to mean "no compression"?)
            break;

        // Run length encoding compression type
        case 1:
            // In AE but never used, used for AO
            break;

        case 2:
            // In AE but never used, used for AO
            break;

        case 3:
            break;

        case 4:
        case 5:
            // Both AO and AE
            break;

        case 6:
            // AE, never seems to get hit for sprites
            abort();
            break;

        case 7:
        case 8:
            // AE, also never seems to be used
            abort();
            break;
        }

        // TODO: Apply pallete to decompressed frame

        return std::vector<Uint8>();
    }

    // ==================================================

    /*static*/ std::vector<std::unique_ptr<Animation>> AnimationFactory::Create(Oddlib::LvlArchive& archive, const std::string& fileName, Uint32 resourceId)
    {
        std::vector < std::unique_ptr<Animation> > r;

        Stream stream(archive.FileByName(fileName)->ChunkById(resourceId)->ReadData());
        AnimSerializer anim(stream);

        return r;
    }

}
