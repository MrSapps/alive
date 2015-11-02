#pragma once

#include "SDL.h"
#include <string>
#include <vector>

namespace Oddlib
{
    class IStream;

    class Path
    {
    public:
        Path(const Path&) = delete;
        Path& operator = (const Path&) = delete;
        Path(IStream& pathChunkStream, 
             Uint32 collisionDataOffset,
             Uint32 objectIndexTableOffset, 
             Uint32 objectDataOffset,
             Uint32 mapXSize, 
             Uint32 mapYSize);

        Uint32 XSize() const;
        Uint32 YSize() const;
        const std::string& CameraFileName(Uint32 x, Uint32 y) const;
    private:
        Uint32 mXSize = 0;
        Uint32 mYSize = 0;

        void ReadCameraMap(IStream& stream);

        struct CollisionItem
        {
            Uint16 mX1;
            Uint16 mY1;
            Uint16 mX2;
            Uint16 mY2;
            Uint16 mType;
            // TODO Actually contains links to previous/next collision
            // item link depending on the type and the line length
            Uint16 mUnknown[5];
        };

        void ReadCollisionItems(IStream& stream, Uint32 numberOfCollisionItems);

        std::vector<std::string> mCameras;

        std::vector<CollisionItem> mCollisionItems;
    };
}