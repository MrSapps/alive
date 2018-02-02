#pragma once

#include <memory>
#include "engine.hpp"
#include "resourcemapper.hpp"
#include "collisionline.hpp"
#include "core/entitymanager.hpp"

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
    WorldState(IAudioController& audioController, ResourceLocator& locator, EntityManager& entityManager);
    WorldState(const WorldState&) = delete;
    WorldState& operator=(const WorldState&) = delete;

public:
    std::deque<std::deque<std::unique_ptr<GridScreen>>> mScreens;

public:
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

public:
    States mState = States::eNone;
    States mReturnToState = States::eNone;

public:
    u32 mModeSwitchTimeout = 0;

public:
    void RenderDebug(AbstractRenderer& rend) const;
    void DebugRayCast(AbstractRenderer& rend, const glm::vec2& from, const glm::vec2& to, u32 collisionType, const glm::vec2& fromDrawOffset = glm::vec2()) const;
    void SetCurrentCamera(const char* cameraName);
    void SetGameCameraToCameraAt(u32 x, u32 y);
    u32 CurrentCameraX() const
    { return mCurrentCameraX; }
    u32 CurrentCameraY() const
    { return mCurrentCameraY; }

public:
    u32 mGlobalFrameCounter = 0;
    EntityManager& mEntityManager;
    std::unique_ptr<PlayFmvState> mPlayFmvState;

private:
    void RenderGrid(AbstractRenderer& rend) const;

private:
    u32 mCurrentCameraX = 0;
    u32 mCurrentCameraY = 0;
};

class World
{
public:
    World(
        Sound& sound,
        InputState& input,
        LoadingIcon& loadingIcon,
        ResourceLocator& locator,
        CoordinateSpace& coords,
        AbstractRenderer& renderer,
        IAudioController& audioController,
        const GameDefinition& selectedGame);
    World(World&&) = delete;
    World& operator=(World&&) = delete;
    ~World();

public:
    EngineStates Update(const InputState& input, CoordinateSpace& coords);
    void Render(AbstractRenderer& rend);

private:
    void LoadSystems();
    void LoadComponents();

private:
    void LoadMap(const std::string& mapName);
    bool LoadMap(const Oddlib::Path& path);
    void UnloadMap(AbstractRenderer& renderer);

private:
    void RenderDebugPathSelection();
    void RenderDebugFmvSelection();

private:
    bool mQuickLoad = false;

private:
    std::unique_ptr<GridMap> mGridMap;
    std::unique_ptr<FmvDebugUi> mFmvDebugUi;
    std::unique_ptr<class GameMode> mGameMode;
    std::unique_ptr<class EditorMode> mEditorMode;
    std::unique_ptr<class AnimationBrowser> mDebugAnimationBrowser;
    Oddlib::UP_Path mPathBeingLoaded;
    up_future_UP_Path mLocatePathFuture;

private:
    Sound& mSound;
    InputState& mInput;
    LoadingIcon& mLoadingIcon;
    ResourceLocator& mLocator;
    AbstractRenderer& mRenderer;

private:
    WorldState mWorldState;
    EntityManager mEntityManager;
};
