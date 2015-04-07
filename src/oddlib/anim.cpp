#include "oddlib/anim.hpp"
#include "oddlib/lvlarchive.hpp"
#include "oddlib/stream.hpp"

namespace Oddlib
{
    AnimSerializer::AnimSerializer(Stream& stream)
    {
        // Read the header
        stream.ReadUInt16(h.iMaxW);
        stream.ReadUInt16(h.iMaxH);
        stream.ReadUInt32(h.iFrameTableOffSet);
        stream.ReadUInt32(h.iPaltSize);
        if (h.iPaltSize == 0)
        {
            // Assume its an Ae file if the palt size is zero, in this case the next Uint32 is
            // actually the palt size.
            mbIsAoFile = false;
            stream.ReadUInt32(h.iPaltSize);
        }

        // Read the palette
        for (auto i = 0u; i < h.iPaltSize; i++)
        {
            // TODO Palt obj
            Uint16 tmp = 0;
            stream.ReadUInt16(tmp);
        }

        // Seek to frame table offset
        stream.Seek(h.iFrameTableOffSet);

        ParseAnimationSets(stream);
    }

    void AnimSerializer::ParseAnimationSets(Stream& stream)
    {
        // Collect all animation sets
        auto hdr = std::make_unique<AnimationHeader>();
        stream.ReadUInt16(hdr->iFps);
        stream.ReadUInt16(hdr->iNumFrames);
        stream.ReadUInt16(hdr->iLoopStartFrame);
        stream.ReadUInt16(hdr->iFlags);
        
        // Read the offsets to each frame info
        hdr->iFrameInfoOffsets.resize(hdr->iNumFrames);
        for (auto& offset : hdr->iFrameInfoOffsets)
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
            if (hdr->iFrameInfoOffsets.empty())
            {
                // Handle a very strange case seen in AO ROPES.BAN, we have a set header that says there are
                // zero frames, then we have a single frame offset *before* the start of the animation!
                stream.Seek(stream.Pos() - 0x14); // FIX ME throws
                ParseAnimationSets(stream);
            }
            else
            {
                // As we said above move to the last frame info offset
                auto offset = hdr->iFrameInfoOffsets.back();
                stream.Seek(offset);

                // Now we want to seek to the end of the frame info where we hope the FrameSet data
                // *really* starts (and why we'll end up where we started after parsing down this list)
                auto frmHdr = std::make_unique<FrameInfoHeader>();

                stream.ReadUInt32(frmHdr->iFrameHeaderOffset);
                stream.ReadUInt32(frmHdr->iMagic);
                // stream.ReadUInt16(frmHdr->points);
                // stream.ReadUInt16(frmHdr->triggers);

                // if (frmHdr->points != 0x3)
                {
                    // I think there is always 3 points?
                    //  abort();
                }
                Uint32 headerDataToSkipSize = 0;
                switch (frmHdr->iMagic)
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

        if (stream.Pos() != h.iFrameTableOffSet)
        {
            ParseAnimationSets(stream);
        }
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
