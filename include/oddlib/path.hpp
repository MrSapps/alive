#pragma once

#include "SDL.h"
#include <string>
#include <vector>
#include <array>
#include <memory>
#include "types.hpp"

namespace Oddlib
{
    class IStream;

    using UP_Path = std::unique_ptr<class Path>;
    class Path
    {
    public:
        struct Point16
        {
            u16 mX;
            u16 mY;
            void Read(IStream& stream);
        };
        static_assert(sizeof(Point16) == 4, "Wrong point size");

        struct MapObject
        {
            // TLV
            u16 mFlags;
            u16 mLength;
            u32 mType;

            // RECT
            Point16 mRectTopLeft;
            Point16 mRectBottomRight;

            // TODO: Set to biggest known size
            // Assume 64 bytes is the biggest length for now
            std::array<u8, 512> mData;
        };

        class Camera
        {
        public:
            explicit Camera(const std::string& name)
                : mName(name)
            {

            }
            std::string mName;
            std::vector<MapObject> mObjects;
        };

        struct Links
        {
            s16 mPrevious;
            s16 mNext;
            void Read(IStream& stream);
        };
        static_assert(sizeof(Links) == 4, "Wrong link size");

        struct CollisionItem
        {
            Point16 mP1;
            Point16 mP2;
            u16 mType;
            Links mLinks[2];
            u16 mLineLength;
            void Read(IStream& stream);
        };
        // sizeof(CollisionItem) != 20 due to padding, since we don't just memcpy things into the POD
        // we had to add up each member individually which is annoying..
        const static auto kCollisionItemSize = 20;
        static_assert(sizeof(Point16) + sizeof(Point16) + sizeof(u16) + sizeof(Links) + sizeof(Links) + sizeof(u16) == kCollisionItemSize, "Wrong collision item size");

        Path(const Path&) = delete;
        Path& operator = (const Path&) = delete;
        Path(IStream& pathChunkStream,
             u32 collisionDataOffset,
             u32 objectIndexTableOffset,
             u32 objectDataOffset,
             u32 mapXSize,
             u32 mapYSize,
             s32 abeSpawnX,
             s32 abeSpawnY,
             bool isAo);

        u32 XSize() const;
        u32 YSize() const;
        s32 AbeSpawnX() const;
        s32 AbeSpawnY() const;

        const Camera& CameraByPosition(u32 x, u32 y) const;
        const std::vector<CollisionItem>& CollisionItems() const { return mCollisionItems; }
        bool IsAo() const { return mIsAo; }
    private:
        void ReadPath(IStream& stream, u32 collisionDataOffset, u32 objectIndexTableOffset, u32 objectDataOffset);
        void ReadCamera(IStream& stream);
        std::vector<u32> ReadOffsetsToPerCameraObjectLists(IStream& stream, u32 objectIndexTableOffset);
        void ReadMapObject(IStream& stream, Path::MapObject& mapObject);
        void ReadMapObjectsForCamera(IStream& stream, Camera& camera);

        u32 mXSize = 0;
        u32 mYSize = 0;

        void ReadCameraArray(IStream& stream);

        void ReadCollisionItemArray(IStream& stream, u32 numberOfCollisionItems);
        void ReadMapObjectsArray(IStream& stream, u32 objectIndexTableOffset);

        std::vector<Camera> mCameras;

        std::vector<CollisionItem> mCollisionItems;
        s32 mAbeSpawnX = -1;
        s32 mAbeSpawnY = -1;
        bool mIsAo = false;
    };
}
