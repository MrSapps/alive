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

class GridScreen
{
public:
    GridScreen(const GridScreen&) = delete;
    GridScreen& operator=(const GridScreen&) = delete;
    GridScreen(const Oddlib::Path::Camera& camera, ResourceLocator& locator);
    ~GridScreen();

public:
    const std::string& FileName() const
    {
        return mFileName;
    }
    void LoadTextures(AbstractRenderer& rend);
    void UnLoadTextures(AbstractRenderer& rend);
    bool HasTexture() const;

    void Render(AbstractRenderer& rend, float x, float y, float w, float h);

private:
    std::string mFileName;
    TextureHandle mCameraTexture;
    TextureHandle mFG1Texture;

    std::unique_ptr<Oddlib::IBits> mCam; // Temp hack to prevent constant reloading of LVLs
    ResourceLocator& mLocator;
};

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
        mData.resize(mXSize*mYSize);
    }

    void Set(u32 x, u32 y, T&& data)
    {
        mData[x * mXSize + y] = std::move(data);
    }

    T& Get(u32 x, u32 y)
    {
        return mData[x * mXSize + y];
    }

    const T& Get(u32 x, u32 y) const
    {
        return mData[x * mXSize + y];
    }
private:
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
public:
    bool LoadMap(const PathInformation& pathInfo);
    void UnloadMap(AbstractRenderer& renderer);

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
