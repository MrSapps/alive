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
    CoordinateSpace& coords,
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
    mGridMap = std::make_unique<GridMap>(coords, *this);
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


    // TODO: Implement
    Debugging().mFnQuickSave = [&]()
    {
        std::filebuf f;
        std::ostream os(&f);
        f.open("save.bin", std::ios::out | std::ios::binary);
        mEntityManager.Serialize(os);
    };

    // TODO: Implement
    Debugging().mFnQuickLoad = [&]()
    {
        mQuickLoad = true;
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

void World::SetCurrentGridScreenFromCAM(const char* camFilename)
{
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
    }
}

u32 World::CurrentGridScreenX() const
{
    return mCurrentGridScreenX;
}

u32 World::CurrentGridScreenY() const
{
    return mCurrentGridScreenY;
}

void World::LoadSystems()
{
    mEntityManager.AddSystem<DebugSystem>();
    mEntityManager.AddSystem<InputSystem>(mInput);
    mEntityManager.AddSystem<CameraSystem>();
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
}

void World::LoadMap(const std::string& mapName)
{
    UnloadMap(mRenderer);

    mLocatePathFuture = mLocator.LocatePath(mapName.c_str());

    mEditorMode->ClearUndoStack();
    mSound.StopAllMusic();
    mLoadingIcon.SetEnabled(true);
    mState = States::eLoadingMap;
}

bool World::LoadMap(const PathInformation& pathInfo)
{
    return mGridMap->LoadMap(pathInfo, mLocator);
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

        if (mGridMap)
        {
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

            // TODO: This needs to be arranged to match the real game which has an ordering of:
            // Update all -> Animate all -> Render all ->  LoadResources/SwitchMap -> ReadInput -> update gnFrame value

            // Physics System
            mEntityManager.With<PhysicsComponent, TransformComponent>([](auto, auto physics, auto transform)
            {
                transform->Add(physics->GetXSpeed(), physics->GetYSpeed());
            });
            // Animation System
            mEntityManager.With<AnimationComponent>([](auto, auto animation)
            {
                animation->Update();
            });
            // Abe system
            mEntityManager.With<AbePlayerControllerComponent, AbeMovementComponent>([](auto, auto controller, auto abe)
            {
                controller->Update();
                abe->Update();
            });
            // Slig system
            mEntityManager.With<SligPlayerControllerComponent, SligMovementComponent>([](auto, auto controller, auto slig)
            {
                controller->Update();
                slig->Update();
            });
            // Input system
            mEntityManager.GetSystem<InputSystem>()->Update();

            if (mQuickLoad)
            {
                std::filebuf f;
                std::istream is(&f);
                f.open("save.bin", std::ios::in | std::ios::binary);
                mEntityManager.Deserialize(is);
                mQuickLoad = false;
            }
        }
    }
        break;

    case States::ePlayFmv:
        if (!mPlayFmvState->Update(input))
        {
            mState = mReturnToState;
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
                        mState = States::eSoundsLoading;
                        mSound.SetMusicTheme(mPathBeingLoaded->mTheme->mMusicTheme.c_str());
                    }
                    else
                    {
                        // No theme set for this path so can't load its music theme
                        mLoadingIcon.SetEnabled(false);
                        mState = States::eInGame;
                    }
                }
            }
            else
            {
                // TODO: Throw ?
                LOG_ERROR("LVL or file in LVL not found");
                
                // HACK: Force to menu
                //mState = States::eFrontEndMenu;

                mState = States::eInGame;

                mLoadingIcon.SetEnabled(false);
            }
        }
        break;

    case States::eSoundsLoading:
        // TODO: Construct sprite sheets for objects that exist in this map
        if (!mSound.IsLoading())
        {
            mState = States::eInGame;
            mSound.HandleMusicEvent("BASE_LINE");
            mLoadingIcon.SetEnabled(false);
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

        if (mGridMap)
        {
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

            mEntityManager.GetSystem<DebugSystem>()->Render(mRenderer);
            mEntityManager.With<AnimationComponent>([this](auto, auto animation) // TODO: should be a system
            {
                animation->Render(mRenderer);
            });
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
        for (const auto& pathMap : mLocator.PathMaps().Map())
        {
            if (ImGui::Button(pathMap.first.c_str()))
            {
                Debugging().mFnLoadPath(pathMap.first.c_str());
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
        mState = States::ePlayFmv;
    }
}

void World::UnloadMap(AbstractRenderer& renderer)
{
    mGridMap->UnloadMap(renderer);
}