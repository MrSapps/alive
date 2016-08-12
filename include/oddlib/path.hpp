#pragma once

#include "SDL.h"
#include <string>
#include <vector>
#include <array>

namespace Oddlib
{
    class IStream;


    class Path
    {
    public:
        struct Point
        {
            u16 mX;
            u16 mY;
        };
        static_assert(sizeof(Point) == 4, "Wrong point size");

        struct MapObject
        {
            // TLV
            u16 mFlags;
            u16 mLength;
            u32 mType;

            // RECT
            Point mRectTopLeft;
            Point mRectBottomRight;

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

        struct CollisionItem
        {
            Point mP1;
            Point mP2;
            u16 mType;
            // TODO Actually contains links to previous/next collision
            // item link depending on the type 
            u16 mUnknown[4];
            u16 mLineLength;
        };
        static_assert(sizeof(CollisionItem) == 20, "Wrong collision item size");

        Path(const Path&) = delete;
        Path& operator = (const Path&) = delete;
        Path(IStream& pathChunkStream, 
             u32 collisionDataOffset,
             u32 objectIndexTableOffset, 
             u32 objectDataOffset,
             u32 mapXSize, 
             u32 mapYSize,
             bool isAo);

        u32 XSize() const;
        u32 YSize() const;
        const Camera& CameraByPosition(u32 x, u32 y) const;
        const std::vector<CollisionItem>& CollisionItems() const { return mCollisionItems; }
        bool IsAo() const { return mIsAo; }
    private:
        u32 mXSize = 0;
        u32 mYSize = 0;

        void ReadCameraMap(IStream& stream);

        void ReadCollisionItems(IStream& stream, u32 numberOfCollisionItems);
        void ReadMapObjects(IStream& stream, u32 objectIndexTableOffset);

        std::vector<Camera> mCameras;

        std::vector<CollisionItem> mCollisionItems;
        bool mIsAo = false;
    };
}