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
class InputReader;
class CoordinateSpace;
class AbstractRenderer;
class IAudioController;
class PlayFmvState;
class FmvDebugUi;
class Sound;
class GridScreen;

class World
{
public:
    World(
        Sound& sound,
        InputReader& input,
        LoadingIcon& loadingIcon,
        ResourceLocator& locator,
        AbstractRenderer& renderer,
        IAudioController& audioController,
        const GameDefinition& selectedGame);
    World(World&&) = delete;
    World& operator=(World&&) = delete;
    ~World();

public:
    enum class States
    {
        eNone,
        eLoadingMap,
        eSoundsLoading,
        ePlayFmv,
        eInGame,
        eFrontEndMenu,
        eToEditor,
        eInEditor,
        eToGame,
        eQuit
    };

public:
    EngineStates Update(const InputReader& input, CoordinateSpace& coords);
    void Render(AbstractRenderer& rend);

public:
    u32 GetCurrentGridScreenX() const;
    u32 GetCurrentGridScreenY() const;
    void SetCurrentGridScreenFromCAM(const char* camFilename);

private:
    void LoadSystems();
    void SetState(States state);
private:
    void LoadMap(const std::string& mapName);
    bool LoadMap(const PathInformation& pathInfo);
    void UnloadMap();

private:
    void RenderDebugPathSelection();
    void RenderDebugFmvSelection();

private:
    u32 mCurrentGridScreenX = 0;
    u32 mCurrentGridScreenY = 0;

private:
    Sound& mSound;
    InputReader& mInput;
    LoadingIcon& mLoadingIcon;
public:
    ResourceLocator& mLocator;  // should be private
private:
    AbstractRenderer& mRenderer;

private:
    std::unique_ptr<class Menu> mMenu;
    std::unique_ptr<class GameMode> mGameMode;
    std::unique_ptr<class EditorMode> mEditorMode;
    std::unique_ptr<class FmvDebugUi> mFmvDebugUi;
    std::unique_ptr<class AnimationBrowser> mDebugAnimationBrowser;
    std::unique_ptr<PathInformation> mPathBeingLoaded;
    up_future_UP_PathInformation mLocatePathFuture;

public:
    u32 mModeSwitchTimeout = 0; // should be private
    u32 mGlobalFrameCounter = 0; // should be private
    States mState = States::eNone;// should be private
    States mReturnToState = States::eNone;// should be private
    EntityManager mEntityManager; // should be private
    std::unique_ptr<PlayFmvState> mPlayFmvState; // should be private
};
