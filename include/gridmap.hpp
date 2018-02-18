#pragma once

#include <memory>
#include <vector>
#include <map>
#include <deque>
#include <iomanip>
#include <sstream>
#include "core/audiobuffer.hpp"
#include "oddlib/path.hpp"
#include "fsm.hpp"
#include "abstractrenderer.hpp"
#include "collisionline.hpp"
#include "imgui/imgui.h"
#include "iterativeforloop.hpp"
#include "core/entitymanager.hpp"

class AbstractRenderer;
class ResourceLocator;
class InputReader;

namespace Oddlib
{
    class LvlArchive;
    class IBits;
}

class Animation;

class Sound;

#define NO_MOVE_OR_MOVE_ASSIGN(x)  x(x&&) = delete; x& operator = (x&&) = delete // TODO: move this out

constexpr u32 kSwitchTimeMs = 300;

struct PathInformation;

class GridScreenData
{
public:
    GridScreenData(const Oddlib::Path::Camera& data)
        : mCameraAndObjects(data)
    {

    }

    // The name of .CAM in the path and its vector of objects to spawn
    Oddlib::Path::Camera mCameraAndObjects;
};

template<class T>
class Vector2D
{
public:
    void Resize(u32 x, u32 y)
    {
        mXSize = x;
        mYSize = y;
        mData.clear();
        mData.resize(mXSize*mYSize);
    }

    void Set(u32 x, u32 y, T&& data)
    {
        mData[To1DIndex(x, y)] = std::move(data);
    }

    T& Get(u32 x, u32 y)
    {
        return mData[To1DIndex(x, y)];
    }

    const T& Get(u32 x, u32 y) const
    {
        return mData[To1DIndex(x, y)];
    }

    u32 XSize() const { return mXSize; }
    u32 YSize() const { return mYSize; }
private:
    u32 To1DIndex(u32 x, u32 y) const
    {
        return (y * mXSize) + x;
    }

    u32 mXSize = 0;
    u32 mYSize = 0;
    std::vector<T> mData;
};

class GridMap
{
public:
    GridMap(const GridMap&) = delete;
    GridMap& operator=(const GridMap&) = delete;
    GridMap(CoordinateSpace& coords, EntityManager& em);
    ~GridMap();

    GridScreenData* GetGridScreen(u32 xIndex, u32 yIndex) const;
    u32 XSize() const { return mMapData.XSize(); }
    u32 YSize() const { return mMapData.YSize(); }
public:
    bool LoadMap(const PathInformation& pathInfo);
private:
    class Loader
    {
    public:
        explicit Loader(GridMap& gm);

    public:
        bool Load(const PathInformation& pathInfo);

    private:
        enum class LoaderStates
        {
            eInit,
            eSetupAndConvertCollisionItems,
            eAllocateCameraMemory,
            eLoadCameras,
            eLoadEntities,
        };

    private:
        void SetState(LoaderStates state);
        void SetupAndConvertCollisionItems(const Oddlib::Path& path);
        void HandleAllocateCameraMemory(const Oddlib::Path& path);
        void HandleLoadCameras(const Oddlib::Path& path);
        void HandleLoadEntities(const PathInformation& pathInfo);
    private:
        GridMap& mGm;
        LoaderStates mState = LoaderStates::eInit;
        IterativeForLoopU32 mXForLoop;
        IterativeForLoopU32 mYForLoop;
        IterativeForLoopU32 mIForLoop;
    };
private:
    Loader mLoader;
    EntityManager& mEntityManager;

    Vector2D<std::unique_ptr<GridScreenData>> mMapData;
};
