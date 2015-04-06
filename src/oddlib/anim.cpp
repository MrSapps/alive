#include "oddlib/anim.hpp"
#include "oddlib/lvlarchive.hpp"
#include "oddlib/stream.hpp"

namespace Oddlib
{
    AnimSerializer::AnimSerializer(Stream& stream)
    {
        BanHeader h;

        // Read the header
        stream.ReadUInt16(h.iMaxW);
        stream.ReadUInt16(h.iMaxH);
        stream.ReadUInt32(h.iFrameTableOffSet);
        stream.ReadUInt32(h.iPaltSize);

        // Read the palette
        for (auto i = 0u; i < h.iPaltSize; i++)
        {
            // TODO Palt obj
            Uint16 tmp = 0;
            stream.ReadUInt16(tmp);
        }

        // Seek to frame table offset
        stream.Seek(h.iFrameTableOffSet);

        // Collect all animation sets
        AnimationHeader hdr;
        stream.ReadUInt16(hdr.iFps);
        stream.ReadUInt16(hdr.iNumFrames);
        stream.ReadUInt16(hdr.iLoopStartFrame);
        stream.ReadUInt16(hdr.iFlags);
        hdr.iFrameInfoOffsets.resize(hdr.iNumFrames);
        for (auto& offset : hdr.iFrameInfoOffsets)
        {
            stream.ReadUInt32(offset);
        }

    }


    /*static*/ std::vector<std::unique_ptr<Animation>> AnimationFactory::Create(Oddlib::LvlArchive& archive, const std::string& fileName, Uint32 resourceId)
    {
        std::vector < std::unique_ptr<Animation> > r;

        Stream stream(archive.FileByName(fileName)->ChunkById(resourceId)->ReadData());
        AnimSerializer anim(stream);

        return r;
    }

}
