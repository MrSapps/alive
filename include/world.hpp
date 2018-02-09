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
        CoordinateSpace& coords,
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
    void SetCurrentGridScreenFromCAM(const char* camFilename);
    u32 CurrentGridScreenX() const;
    u32 CurrentGridScreenY() const;

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
    u32 mCurrentGridScreenX = 0;
    u32 mCurrentGridScreenY = 0;

private:
    std::unique_ptr<GridMap> mGridMap;
    std::unique_ptr<FmvDebugUi> mFmvDebugUi;
    std::unique_ptr<class GameMode> mGameMode;
    std::unique_ptr<class EditorMode> mEditorMode;
    std::unique_ptr<class AnimationBrowser> mDebugAnimationBrowser;
    std::unique_ptr<class Menu> mMenu;
    Oddlib::UP_Path mPathBeingLoaded;
    up_future_UP_Path mLocatePathFuture;

private:
    Sound& mSound;
    InputReader& mInput;
    LoadingIcon& mLoadingIcon;
    ResourceLocator& mLocator;
    AbstractRenderer& mRenderer;

public:
    std::deque<std::deque<std::unique_ptr<GridScreen>>> mScreens; // should be private
    EntityManager mEntityManager; // should be private
    u32 mModeSwitchTimeout = 0; // should be private
    u32 mGlobalFrameCounter = 0; // should be private
    std::unique_ptr<PlayFmvState> mPlayFmvState; // should be private
    States mState = States::eNone;// should be private
    States mReturnToState = States::eNone;// should be private
};
