#include "oddlib/path.hpp"
#include "oddlib/stream.hpp"
#include "logger.hpp"
#include <array>

namespace Oddlib
{
    Path::Path( IStream& pathChunkStream,
                Uint32 numberOfCollisionItems,
                Uint32 objectIndexTableOffset,
                Uint32 objectDataOffset,
                Uint32 mapXSize, Uint32 mapYSize)
     : mXSize(mapXSize), mYSize(mapYSize)
    {
        TRACE_ENTRYEXIT;
        ReadCameraMap(pathChunkStream);
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

        return mCameras[(x * YSize()) + y];
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
            if (!tmpStr.empty())
            {
                tmpStr += ".CAM";
            }
            mCameras.emplace_back(std::move(tmpStr));
        }
    }
}