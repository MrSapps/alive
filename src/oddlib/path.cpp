#include "oddlib/path.hpp"
#include "oddlib/stream.hpp"
#include "logger.hpp"
#include <array>

namespace Oddlib
{
    void Path::Point16::Read(IStream& stream)
    {
        stream.Read(mX);
        stream.Read(mY);
    }

    void Path::Links::Read(IStream& stream)
    {
        stream.Read(mPrevious);
        stream.Read(mNext);
    }

    void Path::CollisionItem::Read(IStream& stream)
    {
        mP1.Read(stream);
        mP2.Read(stream);
        stream.Read(mType);
        mLinks[0].Read(stream);
        mLinks[1].Read(stream);
        stream.Read(mLineLength);
    }

    Path::Path( const std::string& musicThemeName,
                IStream& pathChunkStream,
                u32 collisionDataOffset,
                u32 objectIndexTableOffset,
                u32 objectDataOffset,
                u32 mapXSize, u32 mapYSize,
                bool isAo)
     : mMusicThemeName(musicThemeName), mXSize(mapXSize), mYSize(mapYSize), mIsAo(isAo)
    {
        TRACE_ENTRYEXIT;
        ReadPath(pathChunkStream, collisionDataOffset, objectIndexTableOffset, objectDataOffset);
    }

    void Path::ReadPath(IStream& stream, u32 collisionDataOffset, u32 objectIndexTableOffset, u32 objectDataOffset)
    {
        ReadCameraArray(stream);

        if (collisionDataOffset != 0)
        {
            //pathChunkStream.BinaryDump("PATH.DUMP");
            assert(stream.Pos() + 16 == collisionDataOffset);
        }

        // TODO: Psx data != pc data for Ao
        //const u32 indexTableOffset = (pathChunkStream.Size() - (mCameras.size() * sizeof(u32))) + 16;
        //assert(indexTableOffset == objectIndexTableOffset);

        if (collisionDataOffset != 0)
        {
            const u32 numCollisionDataBytes = objectDataOffset - collisionDataOffset;
            const u32 numCollisionItems = numCollisionDataBytes / kCollisionItemSize;
            ReadCollisionItemArray(stream, numCollisionItems);
            ReadMapObjectsArray(stream, objectIndexTableOffset);
        }
    }

    u32 Path::XSize() const
    {
        return mXSize;
    }

    u32 Path::YSize() const
    {
        return mYSize;
    }

    const Path::Camera& Path::CameraByPosition(u32 x, u32 y) const
    {
        if (x >= XSize() || y >= YSize())
        {
            LOG_ERROR("Out of bounds x:y"
                << std::to_string(x) << " " << std::to_string(y) <<" vs " 
                << std::to_string(XSize()) << " " << std::to_string(YSize()));
            abort();
        }

        return mCameras[(y * XSize()) + x];
    }

    void Path::ReadCameraArray(IStream& stream)
    {
        const u32 numberOfCameras = XSize() * YSize();
        mCameras.reserve(numberOfCameras);

        for (u32 i = 0; i < numberOfCameras; i++)
        {
            ReadCamera(stream);
        }
    }

    void Path::ReadCamera(IStream& stream)
    {
        std::array<char, 8> nameBuffer;
        stream.Read(nameBuffer);
        std::string tmpStr(nameBuffer.data(), nameBuffer.size());
        if (tmpStr[0] != 0)
        {
            tmpStr += ".CAM";
        }
        mCameras.emplace_back(Camera(std::move(tmpStr)));
    }

    void Path::ReadCollisionItemArray(IStream& stream, u32 numberOfCollisionItems)
    {
        mCollisionItems.resize(numberOfCollisionItems);
        for (u32 i = 0; i < numberOfCollisionItems; i++)
        {
            mCollisionItems[i].Read(stream);
        }
    }

    std::vector<u32> Path::ReadOffsetsToPerCameraObjectLists(IStream& stream, u32 objectIndexTableOffset)
    {
        // TODO -16 is for the chunk header, probably shouldn't have this already included in the
        // pathdb, may also apply to collision info
        stream.Seek(objectIndexTableOffset - 16);

        const u32 numberOfCameras = XSize() * YSize();
        std::vector<u32> cameraObjectOffsets;
        cameraObjectOffsets.reserve(numberOfCameras);
        for (u32 i = 0; i < numberOfCameras; i++)
        {
            u32 offset = 0;
            stream.Read(offset);
            cameraObjectOffsets.push_back(offset);
        }

        return cameraObjectOffsets;
    }

    void Path::ReadMapObject(IStream& stream, Path::MapObject& mapObject)
    {
        stream.Read(mapObject.mFlags);
        stream.Read(mapObject.mLength);
        stream.Read(mapObject.mType);

        LOG_INFO("Object TLV: " << mapObject.mType << " " << mapObject.mLength << " " << mapObject.mLength);

        if (mIsAo)
        {
            // Don't know what this is for
            u32 unknownData = 0;
            stream.Read(unknownData);
        }


        stream.Read(mapObject.mRectTopLeft.mX);
        stream.Read(mapObject.mRectTopLeft.mY);

        // Ao duplicated the first two parts of data for some reason
        if (mIsAo)
        {
            u32 duplicatedXY = 0;
            stream.Read(duplicatedXY);
        }

        stream.Read(mapObject.mRectBottomRight.mX);
        stream.Read(mapObject.mRectBottomRight.mY);

        if (mapObject.mLength > 0)
        {
            const u32 len = mapObject.mLength - (sizeof(u16) * (mIsAo ? 12 : 8));
            if (len > 512)
            {
                LOG_ERROR("Map object data length " << mapObject.mLength << " is larger than fixed size");
                abort();
            }
            stream.ReadBytes(mapObject.mData.data(), len);
        }

    }

    void Path::ReadMapObjectsForCamera(IStream& stream, Camera& camera)
    {
        for (;;)
        {
            MapObject mapObject;
            ReadMapObject(stream, mapObject);
            camera.mObjects.emplace_back(mapObject);
            if (mapObject.mFlags & 0x4)
            {
                break;
            }
        }
    }

    void Path::ReadMapObjectsArray(IStream& stream, u32 objectIndexTableOffset)
    {
        const size_t collisionEndPos = stream.Pos();

        // Read the pointers to the object list for each camera
        const std::vector<u32> cameraObjectOffsets = ReadOffsetsToPerCameraObjectLists(stream, objectIndexTableOffset);
        
        // Now load the objects for each camera
        for (auto i = 0u; i < cameraObjectOffsets.size(); i++)
        {
            // If max u32/-1 then it means there are no objects for this camera
            const auto objectsOffset = cameraObjectOffsets[i];
            if (objectsOffset != 0xFFFFFFFF)
            {
                stream.Seek(collisionEndPos + objectsOffset);
                ReadMapObjectsForCamera(stream, mCameras[i]);
            }
        }
    }
}