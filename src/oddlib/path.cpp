#include "oddlib/path.hpp"
#include "oddlib/stream.hpp"
#include "logger.hpp"
#include <array>

namespace Oddlib
{
    Path::Path( IStream& pathChunkStream,
                Uint32 collisionDataOffset,
                Uint32 /*objectIndexTableOffset*/,
                Uint32 objectDataOffset,
                Uint32 mapXSize, Uint32 mapYSize)
     : mXSize(mapXSize), mYSize(mapYSize)
    {
        TRACE_ENTRYEXIT;
        ReadCameraMap(pathChunkStream);
        if (collisionDataOffset != 0)
        {
            const auto numCollisionDataBytes = objectDataOffset - collisionDataOffset;
            const auto numCollisionItems = numCollisionDataBytes / sizeof(CollisionItem);
            ReadCollisionItems(pathChunkStream, numCollisionItems);
        }
    }

    Uint32 Path::XSize() const
    {
        return mXSize;
    }

    Uint32 Path::YSize() const
    {
        return mYSize;
    }

    const std::string& Path::CameraFileName(Uint32 x, Uint32 y) const
    {
        if (x >= XSize() || y >= YSize())
        {
            LOG_ERROR("Out of bounds x:y"
                << std::to_string(x) << " " << std::to_string(y) <<" vs " 
                << std::to_string(XSize()) << " " << std::to_string(YSize()));
        }

        return mCameras[(y * XSize()) + x];
    }

    void Path::ReadCameraMap(IStream& stream)
    {
        const Uint32 numberOfCameras = XSize() * YSize();
        mCameras.reserve(numberOfCameras);

        std::array<Uint8, 8> nameBuffer;
        for (Uint32 i = 0; i < numberOfCameras; i++)
        {
            stream.ReadBytes(nameBuffer.data(), nameBuffer.size());
            std::string tmpStr(reinterpret_cast<const char*>(nameBuffer.data()), nameBuffer.size());
            if (tmpStr[0] != 0)
            {
                tmpStr += ".CAM";
            }
            mCameras.emplace_back(std::move(tmpStr));
        }
    }

    void Path::ReadCollisionItems(IStream& stream, Uint32 numberOfCollisionItems)
    {
        for (Uint32 i = 0; i < numberOfCollisionItems; i++)
        {
            CollisionItem tmp = {};
            stream.ReadUInt16(tmp.mX1);
            stream.ReadUInt16(tmp.mY1);
            stream.ReadUInt16(tmp.mX2);
            stream.ReadUInt16(tmp.mY2);
            stream.ReadUInt16(tmp.mType);
            for (int j = 0; j < 5; j++)
            {
                stream.ReadUInt16(tmp.mUnknown[j]);
            }
            mCollisionItems.emplace_back(tmp);
        }
    }

}