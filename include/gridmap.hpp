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
#include "proxy_sqrat.hpp"
#include "mapobject.hpp"
#include "imgui/imgui.h"
#include "iterativeforloop.hpp"

class AbstractRenderer;
class ResourceLocator;
class InputState;

namespace Oddlib { class LvlArchive; class IBits; }

class Animation;

class IMap
{
public:
    virtual ~IMap() = default;
    virtual const CollisionLines& Lines() const = 0;
};

class Sound;

class Level
{
public:
    Level(Level&&) = delete;
    Level& operator = (Level&&) = delete;
    ~Level()
    {
        TRACE_ENTRYEXIT;
    }
    Level(ResourceLocator& locator);
    bool LoadMap(const Oddlib::Path& path);
    void Update(const InputState& input, CoordinateSpace& coords);
    void Render(AbstractRenderer& rend);
private:
    void RenderDebugPathSelection();
    std::unique_ptr<class GridMap> mMap;
    ResourceLocator& mLocator;
};

class GridScreen
{
public:
    GridScreen(const GridScreen&) = delete;
    GridScreen& operator = (const GridScreen&) = delete;
    GridScreen(const Oddlib::Path::Camera& camera, ResourceLocator& locator);
    ~GridScreen();
    const std::string& FileName() const { return mFileName; }
    void LoadTextures(AbstractRenderer& rend);
    void UnLoadTextures(AbstractRenderer& rend);
    bool hasTexture() const;
    const Oddlib::Path::Camera &getCamera() const { return mCamera; }
    void Render(AbstractRenderer& rend, float x, float y, float w, float h);
private:
    std::string mFileName;
    TextureHandle mTexHandle;
    TextureHandle mTexHandle2;

    // TODO: This is not the in-game format
    Oddlib::Path::Camera mCamera;

    // Temp hack to prevent constant reloading of LVLs
    std::unique_ptr<Oddlib::IBits> mCam;

    ResourceLocator& mLocator;
};


#define NO_MOVE_OR_MOVE_ASSIGN(x)  x(x&&) = delete; x& operator = (x&&) = delete


class GridMapState
{
public:
    GridMapState() = default;
    NO_MOVE_OR_MOVE_ASSIGN(GridMapState);

    glm::vec2 kVirtualScreenSize;
    glm::vec2 kCameraBlockSize;
    glm::vec2 kCamGapSize;
    glm::vec2 kCameraBlockImageOffset;

    const int mEditorGridSizeX = 25;
    const int mEditorGridSizeY = 20;

    glm::vec2 mCameraPosition;
    MapObject* mCameraSubject = nullptr;

    std::deque<std::deque<std::unique_ptr<GridScreen>>> mScreens;

    // CollisionLine contains raw pointers to other CollisionLine objects. Hence the vector
    // has unique_ptrs so that adding or removing to this vector won't cause the raw pointers to dangle.
    CollisionLines mCollisionItems;
    std::vector<std::unique_ptr<MapObject>> mObjs;

    enum class eStates
    {
        eInGame,
        eToEditor,
        eEditor,
        eToGame
    };

    eStates mState = eStates::eInGame;
    u32 mModeSwitchTimeout = 0;

    void RenderDebug(AbstractRenderer& rend) const;
    void DebugRayCast(AbstractRenderer& rend, const glm::vec2& from, const glm::vec2& to, u32 collisionType, const glm::vec2& fromDrawOffset = glm::vec2()) const;
private:
    void RenderGrid(AbstractRenderer& rend) const;
};

constexpr u32 kSwitchTimeMs = 300;

class GridMap : public IMap
{
public:
    GridMap(const GridMap&) = delete;
    GridMap& operator = (const GridMap&) = delete;
    GridMap();
    ~GridMap();
    bool LoadMap(const Oddlib::Path& path, ResourceLocator& locator);
    void Update(const InputState& input, CoordinateSpace& coords);
    void Render(AbstractRenderer& rend) const;
    static void RegisterScriptBindings();
private:
    class Loader
    {
    public:
        Loader(GridMap& gm);
        bool Load(const Oddlib::Path& path, ResourceLocator& locator);
    private:
        void SetupAndConvertCollisionItems(const Oddlib::Path& path);
        void HandleAllocateCameraMemory(const Oddlib::Path& path);
        void HandleLoadCameras(const Oddlib::Path& path, ResourceLocator& locator);
        void HandleObjectLoaderScripts(ResourceLocator& locator);
        void HandleLoadObjects(const Oddlib::Path& path, ResourceLocator& locator);
        void HandleHackAbeIntoValidCamera(ResourceLocator& locator);

        GridMap& mGm;
        enum class LoaderStates
        {
            eInit,
            eSetupAndConvertCollisionItems,
            eAllocateCameraMemory,
            eLoadCameras,
            eObjectLoaderScripts,
            eLoadObjects,
            eHackToPlaceAbeInValidCamera,
        };
        
        LoaderStates mState = LoaderStates::eInit;
        IterativeForLoopU32 mXForLoop;
        IterativeForLoopU32 mYForLoop;
        IterativeForLoopU32 mIForLoop;
        UP_MapObject mMapObjectBeingLoaded;

        void SetState(LoaderStates state);
    };
    Loader mLoader;

    MapObject* GetMapObject(s32 x, s32 y, const char* type);
    
    void UpdateToEditorOrToGame(const InputState& input, CoordinateSpace& coords);
    void RenderToEditorOrToGame(AbstractRenderer& rend) const;

    virtual const CollisionLines& Lines() const override final { return mMapState.mCollisionItems; }

    void ConvertCollisionItems(const std::vector<Oddlib::Path::CollisionItem>& items);

    GridMapState mMapState;
    std::unique_ptr<class EditorMode> mEditorMode;
    std::unique_ptr<class GameMode> mGameMode;
    InstanceBinder<class GridMap> mScriptInstance;
};
