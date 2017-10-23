#pragma once

#include <memory>
#include "engine.hpp"
#include "resourcemapper.hpp"
#include "collisionline.hpp"

namespace Oddlib
{
    class Path;
}

class GridMap;
class InputState;
class CoordinateSpace;
class AbstractRenderer;
class IAudioController;
class PlayFmvState;
class FmvDebugUi;
class Sound;
class GridScreen;

class WorldState
{
public:
    WorldState(const WorldState&) = delete;
    WorldState& operator = (const WorldState&) = delete;
    WorldState() {} 

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

    enum class States
    {
        eNone,
        eLoadingMap,
        eSoundsLoading,
        ePlayFmv,
        eInGame,
        eToEditor,
        eInEditor,
        eToGame,
        eQuit
    };

    States mState = States::eNone;
    States mReturnToState = States::eNone;

    u32 mModeSwitchTimeout = 0;

    void RenderDebug(AbstractRenderer& rend) const;
    void DebugRayCast(AbstractRenderer& rend, const glm::vec2& from, const glm::vec2& to, u32 collisionType, const glm::vec2& fromDrawOffset = glm::vec2()) const;
private:
    void RenderGrid(AbstractRenderer& rend) const;
};

class World
{
public:
    World(World&&) = delete;
    World& operator = (World&&) = delete;
    ~World();
    World(
        IAudioController& audioController,
        ResourceLocator& locator,
        CoordinateSpace& coords,
        AbstractRenderer& renderer,
        const GameDefinition& selectedGame,
        Sound& sound,
        LoadingIcon& loadingIcon);

    EngineStates Update(const InputState& input, CoordinateSpace& coords);
    void Render(AbstractRenderer& rend);
private:
    void LoadMap(const std::string& mapName);
    bool LoadMap(const Oddlib::Path& path);
    void UnloadMap(AbstractRenderer& renderer);

    void RenderDebugPathSelection();
    void RenderDebugFmvSelection();

    std::unique_ptr<GridMap> mGridMap;
    std::unique_ptr<PlayFmvState> mPlayFmvState;
    std::unique_ptr<FmvDebugUi> mFmvDebugUi;
    std::unique_ptr<class EditorMode> mEditorMode;
    std::unique_ptr<class GameMode> mGameMode;

    up_future_UP_Path mLocatePathFuture;
    Oddlib::UP_Path mPathBeingLoaded;

    WorldState mWorldState;

    ResourceLocator& mLocator;
    Sound& mSound;
    AbstractRenderer& mRenderer;
    LoadingIcon& mLoadingIcon;
};
