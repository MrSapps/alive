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
             Uint32 numberOfCollisionItems,
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

        std::vector<std::string> mCameras;
    };
}