#include "world.hpp"
#include "debug.hpp"
#include "logger.hpp"
#include "gridmap.hpp"
#include "resourcemapper.hpp"
#include "fmv.hpp"
#include "input.hpp"
#include "sound.hpp"
#include "editormode.hpp"
#include "gamemode.hpp"
#include "animationbrowser.hpp"
#include "menu.hpp"

#include "core/systems/debugsystem.hpp"
#include "core/systems/inputsystem.hpp"
#include "core/systems/camerasystem.hpp"
#include "core/systems/resourcesystem.hpp"
#include "core/systems/collisionsystem.hpp"
#include "core/systems/gridmapsystem.hpp"

#include "core/components/cameracomponent.hpp"
#include "core/components/physicscomponent.hpp"
#include "core/components/transformcomponent.hpp"
#include "core/components/animationcomponent.hpp"
#include "core/components/abemovementcomponent.hpp"
#include "core/components/sligmovementcomponent.hpp"

template<class T>
static inline bool FutureIsDone(T& future)
{
    return future->wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}

World::World(
    Sound& sound,
    InputReader& input,
    LoadingIcon& loadingIcon,
    ResourceLocator& locator,
    AbstractRenderer& renderer,
    IAudioController& audioController,
    const GameDefinition& selectedGame) : mSound(sound),
                                          mInput(input),
                                          mLoadingIcon(loadingIcon),
                                          mLocator(locator),
                                          mRenderer(renderer)
{
    LoadSystems();

    mMenu = std::make_unique<Menu>(mEntityManager);

    mGameMode = std::make_unique<GameMode>(*this);
    mEditorMode = std::make_unique<EditorMode>(*this);
    mFmvDebugUi = std::make_unique<FmvDebugUi>(locator);
    mPlayFmvState = std::make_unique<PlayFmvState>(audioController, locator);
    mDebugAnimationBrowser = std::make_unique<AnimationBrowser>(mEntityManager, locator);

    Debugging().AddSection([&]()
    {
        RenderDebugPathSelection();
    });

    Debugging().AddSection([&]()
    {
        RenderDebugFmvSelection();
    });

    // Debugging - reload path and load next path
    static std::string currentPathName;
    static s32 nextPathIndex;

    Debugging().mFnLoadPath = [&](const char* name)
    {
        LoadMap(name);
    };

    Debugging().mFnNextPath = [&]()
    {
        static s32 idx;
        LoadMap(mLocator.PathMaps().NextPathName(idx));
    };

    Debugging().mFnReloadPath = [&]()
    {
        // TODO: Fix me
        //LoadMap(mLevel->MapName());
    };

    Debugging().mFnQuickSave = [&]()
    {
        std::filebuf f;
        std::ostream os(&f);
        f.open("quicksave.bin", std::ios::out | std::ios::binary);
        if (os.good())
        {
            mEntityManager.Serialize(os);
        }
    };

    Debugging().mFnQuickLoad = [&]()
    {
        std::filebuf f;
        std::istream is(&f);
        f.open("quicksave.bin", std::ios::in | std::ios::binary);
        if (is.good())
        {
            mEntityManager.Deserialize(is);
        }
    };

    // TODO: Get the starting map from selectedGame
    std::ignore = selectedGame;
    std::ignore = nextPathIndex;

    // TODO: Can be removed ?
    //const std::string gameScript = mResourceLocator.LocateScript(initScriptName).get();

    //LoadMap("STPATH_1");
    LoadMap("BAPATH_1");
}

World::~World()
{
    TRACE_ENTRYEXIT;

    UnloadMap(mRenderer);
}

void World::SetCurrentGridScreenFromCAM(const char* /*camFilename*/)
{
    /* TODO: Grid map system
    for (auto x = 0u; x < mScreens.size(); x++)
    {
        for (auto y = 0u; y < mScreens[x].size(); y++)
        {
            if (mScreens[x][y]->FileName() == camFilename)
            {
                mCurrentGridScreenX = x;
                mCurrentGridScreenY = y;
                return;
            }
        }
    }*/
}

u32 World::GetCurrentGridScreenX() const
{
    return mCurrentGridScreenX;
}

u32 World::GetCurrentGridScreenY() const
{
    return mCurrentGridScreenY;
}

void World::LoadSystems()
{
    mEntityManager.AddSystem<DebugSystem>();
    mEntityManager.AddSystem<InputSystem>(mInput);
    mEntityManager.AddSystem<CameraSystem>();
    mEntityManager.AddSystem<GridmapSystem>(mRenderer);
    mEntityManager.AddSystem<ResourceSystem>(mLocator);
    mEntityManager.AddSystem<CollisionSystem>();

    mEntityManager.RegisterComponent<CameraComponent>();
    mEntityManager.RegisterComponent<PhysicsComponent>();
    mEntityManager.RegisterComponent<TransformComponent>();
    mEntityManager.RegisterComponent<AnimationComponent>();
    mEntityManager.RegisterComponent<AbeMovementComponent>();
    mEntityManager.RegisterComponent<SligMovementComponent>();
    mEntityManager.RegisterComponent<AbePlayerControllerComponent>();
    mEntityManager.RegisterComponent<SligPlayerControllerComponent>();
    mEntityManager.RegisterComponent<GridMapScreenComponent>();

    mEntityManager.ResolveSystemDependencies();
}

void World::SetState(World::States state)
{
    if (mState != state)
    {
        mState = state;
        if (mState == States::eInGame)
        {
            CameraSystem* cameraSystem = mEntityManager.GetSystem<CameraSystem>();
            mEntityManager.GetSystem<GridmapSystem>()->MoveToCamera(
                static_cast<u32>(mPathBeingLoaded->mPath->AbeSpawnX() / cameraSystem->mVirtualScreenSize.x),
                static_cast<u32>(mPathBeingLoaded->mPath->AbeSpawnY() / cameraSystem->mVirtualScreenSize.y));
            mLoadingIcon.SetEnabled(false);
            mSound.HandleMusicEvent("BASE_LINE");
        }
    }
}

void World::LoadMap(const std::string& mapName)
{
    UnloadMap(mRenderer);

    mLocatePathFuture = mLocator.LocatePath(mapName.c_str());

    mEditorMode->ClearUndoStack();
    mSound.StopAllMusic();
    mLoadingIcon.SetEnabled(true);
    SetState(States::eLoadingMap);
}

bool World::LoadMap(const PathInformation& pathInfo)
{
    return mEntityManager.GetSystem<GridmapSystem>()->LoadMap(pathInfo);
}

void World::UnloadMap(AbstractRenderer& renderer)
{
    mEntityManager.GetSystem<GridmapSystem>()->UnloadMap(renderer);
    mEntityManager.Clear();
    // TODO: Grid map system
    //mScreens.clear();
    auto collisionSystem = mEntityManager.GetSystem<CollisionSystem>();
    if (collisionSystem)
    {
        collisionSystem->Clear();
    }
}

EngineStates World::Update(const InputReader& input, CoordinateSpace& coords)
{
    switch (mState)
    {
    case States::eToEditor:
    case States::eToGame:
    case States::eInGame:
    case States::eFrontEndMenu:
    case States::eInEditor:
    {
        mGlobalFrameCounter++;

        // Don't show debug UI if we're in game and the game is paused
        bool hideDebug = false;
        if (mState == States::eInGame && mGameMode->State() == GameMode::ePaused)
        {
            hideDebug = true;
        }

        if (!hideDebug)
        {
            Debugging().Update(input);
            if (Debugging().mBrowserUi.animationBrowserOpen)
            {
                mDebugAnimationBrowser->Update(input, coords);
            }
        }

        if (mState == States::eInEditor)
        {
            mEditorMode->Update(input, coords);
        }
        else if (mState == States::eInGame)
        {
            mGameMode->Update(input, coords);
        }
        else if (mState == States::eFrontEndMenu)
        {
            mMenu->Update();
        }
        else
        {
            if (mState == States::eToEditor)
            {
                coords.SetScreenSize(glm::vec2(coords.Width(), coords.Height()) * mEditorMode->mEditorCamZoom);
                if (SDL_TICKS_PASSED(SDL_GetTicks(), mModeSwitchTimeout))
                {
                    mState = States::eInEditor;
                }
            }
            else if (mState == States::eToGame)
            {
                const auto cameraSystem = mEntityManager.GetSystem<CameraSystem>();
                coords.SetScreenSize(cameraSystem->mVirtualScreenSize);
                if (SDL_TICKS_PASSED(SDL_GetTicks(), mModeSwitchTimeout))
                {
                    mState = States::eInGame;
                }
            }
            coords.SetCameraPosition(mEntityManager.GetSystem<CameraSystem>()->mCameraPosition);
        }
    }
    break;

    case States::ePlayFmv:
        if (!mPlayFmvState->Update(input))
        {
            SetState(mReturnToState);
        }
        break;

    case States::eLoadingMap:
        if (mLocatePathFuture && FutureIsDone(mLocatePathFuture))
        {
            mPathBeingLoaded = mLocatePathFuture->get(); // Will re-throw anything that was thrown during the async processing
            mLocatePathFuture = nullptr;
        }

        if (!mLocatePathFuture)
        {
            if (mPathBeingLoaded && mPathBeingLoaded->mPath)
            {
                // Note: This is iterative loading which happens in the main thread
                if (LoadMap(*mPathBeingLoaded))
                {
                    if (mPathBeingLoaded->mTheme)
                    {
                        SetState(States::eSoundsLoading);
                        mSound.SetMusicTheme(mPathBeingLoaded->mTheme->mMusicTheme.c_str());
                    }
                    else
                    {
                        // No theme set for this path so can't load its music theme
                        SetState(States::eInGame);
                    }
                }
            }
            else
            {
                // TODO: Throw ?
                LOG_ERROR("LVL or file in LVL not found");

                // HACK: Force to menu
                //mState = States::eFrontEndMenu;

                SetState(States::eInGame);
            }
        }
        break;

    case States::eSoundsLoading:
        // TODO: Construct sprite sheets for objects that exist in this map
        if (!mSound.IsLoading())
        {
            SetState(States::eInGame);
        }
        break;

    case States::eQuit:
        return EngineStates::eQuit;

    case States::eNone:
        break;
    }

    mSound.Update();

    return EngineStates::eRunSelectedGame;
}

void World::Render(AbstractRenderer& /*rend*/)
{
    switch (mState)
    {
    case States::eInGame:
    case States::eToEditor:
    case States::eToGame:
    case States::eFrontEndMenu:
    case States::eInEditor:
        mRenderer.Clear(0.4f, 0.4f, 0.4f);

        Debugging().Render(mRenderer);
        if (Debugging().mBrowserUi.animationBrowserOpen)
        {
            mDebugAnimationBrowser->Render(mRenderer);
        }

        mPlayFmvState->RenderDebugSubsAndFontAtlas(mRenderer);

        if (mState == States::eInEditor)
        {
            mEditorMode->Render(mRenderer);
        }
        else if (mState == States::eInGame)
        {
            mGameMode->Render(mRenderer);
        }
        else if (mState == States::eFrontEndMenu)
        {
            mMenu->Render(mRenderer);
        }
        else
        {
            mEditorMode->Render(mRenderer);
        }
        break;

    case States::ePlayFmv:
        mPlayFmvState->Render(mRenderer);
        break;

    case States::eQuit:
        mRenderer.Clear(0.0f, 0.0f, 0.0f);
        break;

    case States::eLoadingMap:
        // TODO: Need to keep rendering the last camera/objects while loading
        // and then run the transition effect
        mRenderer.Clear(0.0f, 0.0f, 0.0f);
        break;

    case States::eSoundsLoading:
        break;

    case States::eNone:
        break;
    }
}

void World::RenderDebugPathSelection()
{
    if (ImGui::CollapsingHeader("Maps"))
    {
        static int button = 0;
        if (ImGui::RadioButton("No filter", button == 0))
        {
            button = 0;
        }
        else if (ImGui::RadioButton("AePc filter", button == 1))
        {
            button = 1;
        }

        for (const auto& pathMap : mLocator.PathMaps().Map())
        {
            bool add = false;
            if (button == 0)
            {
                add = true;
            }
            else
            {
                for (const auto& loc : pathMap.second.mLocations)
                {
                    if (loc.mDataSetName == "AePc")
                    {
                        add = true;
                        break;
                    }
                }
            }

            if (add)
            {
                if (ImGui::Selectable(pathMap.first.c_str()))
                {
                    Debugging().mFnLoadPath(pathMap.first.c_str());
                }
            }
        }
    }
}

void World::RenderDebugFmvSelection()
{
    if (mFmvDebugUi->Ui())
    {
        mPlayFmvState->Play(mFmvDebugUi->FmvName().c_str(), mFmvDebugUi->FmvFileLocation());
        mReturnToState = mState;
        SetState(States::ePlayFmv);
    }
}