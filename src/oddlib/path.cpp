#include "oddlib/path.hpp"
#include "oddlib/stream.hpp"
#include "logger.hpp"
#include <array>

namespace Oddlib
{
    Path::Path( IStream& pathChunkStream,
                Uint32 collisionDataOffset,
                Uint32 objectIndexTableOffset,
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
            ReadMapObjects(pathChunkStream, objectIndexTableOffset);
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
            stream.ReadUInt16(tmp.mP1.mX);
            stream.ReadUInt16(tmp.mP1.mY);
            stream.ReadUInt16(tmp.mP2.mX);
            stream.ReadUInt16(tmp.mP2.mY);
            stream.ReadUInt16(tmp.mType);
            for (int j = 0; j < 4; j++)
            {
                stream.ReadUInt16(tmp.mUnknown[j]);
            }
            stream.ReadUInt16(tmp.mLineLength);
            mCollisionItems.emplace_back(tmp);
        }
    }

    void Path::ReadMapObjects(IStream& stream, Uint32 objectIndexTableOffset)
    {
        const size_t collisionEnd = stream.Pos();

        // TODO -16 is for the chunk header, probably shouldn't have this already included in the
        // pathdb, may also apply to collision info
        stream.Seek(objectIndexTableOffset-16);

        // Read the pointers to the object list for each camera
        const Uint32 numberOfCameras = XSize() * YSize();
        std::vector<Uint32> cameraObjectOffsets;
        cameraObjectOffsets.reserve(numberOfCameras);
        for (Uint32 i = 0; i < numberOfCameras; i++)
        {
            Uint32 offset = 0;
            stream.ReadUInt32(offset);
            cameraObjectOffsets.push_back(offset);
        }

        // Now load the objects for each camera
        for (const Uint32 objectsOffset : cameraObjectOffsets)
        {
            // If max Uint32/-1 then it means there are no objects for this camera
            if (objectsOffset != 0xFFFFFFFF)
            {
                stream.Seek(collisionEnd + objectsOffset);
                Uint16 lastFlags = 0;
              //  do
              //  {
                    MapObject mapObject;
                    stream.ReadUInt16(mapObject.mFlags);
                    stream.ReadUInt16(mapObject.mLength);
                    stream.ReadUInt16(mapObject.mType);

                    // TODO: Fix reading - not the same between AO and AE 
                    // plus data structure is slightly wrong

                    stream.ReadUInt16(mapObject.mRectTopLeft.mX);
                    stream.ReadUInt16(mapObject.mRectTopLeft.mY);
                    stream.ReadUInt16(mapObject.mRectBottomRight.mX);
                    stream.ReadUInt16(mapObject.mRectBottomRight.mY);

                    if (mapObject.mLength > 64)
                    {
                        LOG_ERROR("Map object data length " << mapObject.mLength << " is larger than fixed size");
                    }

                    stream.ReadBytes(mapObject.mData.data(), mapObject.mLength);
                    lastFlags = mapObject.mFlags;

               // } while (!(lastFlags & 0x4));
                    
            }
        }

    }

}