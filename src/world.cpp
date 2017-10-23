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

void WorldState::RenderGrid(AbstractRenderer& rend) const
{
    const int gridLineCountX = static_cast<int>((rend.ScreenSize().x / mEditorGridSizeX));
    for (int x = -gridLineCountX; x < gridLineCountX; x++)
    {
        const glm::vec2 worldPos(rend.CameraPosition().x + (x * mEditorGridSizeX) - (static_cast<int>(rend.CameraPosition().x) % mEditorGridSizeX), 0);
        const glm::vec2 screenPos = rend.WorldToScreen(worldPos);
        rend.Line(ColourU8{ 255, 255, 255, 30 }, screenPos.x, 0, screenPos.x, static_cast<f32>(rend.Height()), 2.0f, AbstractRenderer::eLayers::eEditor, AbstractRenderer::eNormal, AbstractRenderer::eScreen);
    }

    const int gridLineCountY = static_cast<int>((rend.ScreenSize().y / mEditorGridSizeY));
    for (int y = -gridLineCountY; y < gridLineCountY; y++)
    {
        const glm::vec2 screenPos = rend.WorldToScreen(glm::vec2(0, rend.CameraPosition().y + (y * mEditorGridSizeY) - (static_cast<int>(rend.CameraPosition().y) % mEditorGridSizeY)));
        rend.Line(ColourU8{ 255, 255, 255, 30 }, 0, screenPos.y, static_cast<f32>(rend.Width()), screenPos.y, 2.0f, AbstractRenderer::eLayers::eEditor, AbstractRenderer::eNormal, AbstractRenderer::eScreen);
    }
}

void WorldState::RenderDebug(AbstractRenderer& rend) const
{
    //rend.SetActiveLayer(AbstractRenderer::eEditor);

    // Draw collisions
    if (Debugging().mCollisionLines)
    {
        CollisionLine::Render(rend, mCollisionItems);
    }

    // Draw grid
    if (Debugging().mGrid)
    {
        RenderGrid(rend);
    }

    // Draw objects
    if (Debugging().mObjectBoundingBoxes)
    {
        for (auto x = 0u; x < mScreens.size(); x++)
        {
            for (auto y = 0u; y < mScreens[x].size(); y++)
            {
                GridScreen* screen = mScreens[x][y].get();
                if (!screen)
                {
                    continue;
                }
                const Oddlib::Path::Camera& cam = screen->getCamera();
                for (size_t i = 0; i < cam.mObjects.size(); ++i)
                {
                    const Oddlib::Path::MapObject& obj = cam.mObjects[i];

                    glm::vec2 topLeft = glm::vec2(obj.mRectTopLeft.mX, obj.mRectTopLeft.mY);
                    glm::vec2 bottomRight = glm::vec2(obj.mRectBottomRight.mX, obj.mRectBottomRight.mY);

                    glm::vec2 objPos = rend.WorldToScreen(glm::vec2(topLeft.x, topLeft.y));
                    glm::vec2 objSize = rend.WorldToScreen(glm::vec2(bottomRight.x, bottomRight.y)) - objPos;

                    rend.Rect(
                        objPos.x, objPos.y,
                        objSize.x, objSize.y,
                        AbstractRenderer::eLayers::eEditor, ColourU8{ 255, 255, 255, 255 }, AbstractRenderer::eNormal, AbstractRenderer::eScreen);

                }
            }
        }
    }
}

void WorldState::DebugRayCast(AbstractRenderer& rend, const glm::vec2& from, const glm::vec2& to, u32 collisionType, const glm::vec2& fromDrawOffset) const
{
    if (Debugging().mRayCasts)
    {
        Physics::raycast_collision collision;
        if (CollisionLine::RayCast<1>(mCollisionItems, from, to, { collisionType }, &collision))
        {
            const glm::vec2 fromDrawPos = rend.WorldToScreen(from + fromDrawOffset);
            const glm::vec2 hitPos = rend.WorldToScreen(collision.intersection);

            rend.Line(ColourU8{ 255, 0, 255, 255 },
                fromDrawPos.x, fromDrawPos.y,
                hitPos.x, hitPos.y,
                2.0f,
                AbstractRenderer::eLayers::eEditor,
                AbstractRenderer::eBlendModes::eNormal,
                AbstractRenderer::eCoordinateSystem::eScreen);
        }
    }
}

template<class T>
static inline bool FutureIsDone(T& future)
{
    return future->wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}

World::World(
    IAudioController& audioController,
    ResourceLocator& locator,
    CoordinateSpace& coords,
    AbstractRenderer& renderer,
    const GameDefinition& selectedGame,
    Sound& sound,
    LoadingIcon& loadingIcon)
  : mLocator(locator),
    mSound(sound),
    mRenderer(renderer),
    mLoadingIcon(loadingIcon)
{
    mGridMap = std::make_unique<GridMap>(coords, mWorldState);
    
    mEditorMode = std::make_unique<EditorMode>(mWorldState);
    mGameMode = std::make_unique<GameMode>(mWorldState);

    mPlayFmvState = std::make_unique<PlayFmvState>(audioController, locator);
    mFmvDebugUi = std::make_unique<FmvDebugUi>(locator);

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

    Debugging().fnLoadPath = [&](const char* name)
    {
        LoadMap(name);
    };

    Debugging().mFnNextPath = [&]()
    {
        s32 idx = 0;
        for (const auto& pathMap : mLocator.PathMaps())
        {
            if (idx == nextPathIndex)
            {
                LoadMap(pathMap.first.c_str());
                return;
            }
            idx++;
        }
    };

    Debugging().mFnReloadPath = [&]()
    {
        // TODO: Fix me
        //LoadMap(mLevel->MapName());
    };

    // TODO: Get the starting map from selectedGame
    std::ignore = selectedGame;

    // TODO: Can be removed ?
    //const std::string gameScript = mResourceLocator.LocateScript(initScriptName).get();

    LoadMap("BAPATH_1");
}

World::~World()
{
    TRACE_ENTRYEXIT;

    UnloadMap(mRenderer);
}

void World::LoadMap(const std::string& mapName)
{
    UnloadMap(mRenderer);

    mLocatePathFuture = mLocator.LocatePath(mapName.c_str());

    mEditorMode->ClearUndoStack();
    mSound.StopAllMusic();
    mLoadingIcon.SetEnabled(true);
    mWorldState.mState = WorldState::States::eLoadingMap;
}

bool World::LoadMap(const Oddlib::Path& path)
{
    return mGridMap->LoadMap(path, mLocator);
}

EngineStates World::Update(const InputState& input, CoordinateSpace& coords)
{
    switch (mWorldState.mState)
    {
    case WorldState::States::eToEditor:
    case WorldState::States::eToGame:
    case WorldState::States::eInGame:
    case WorldState::States::eInEditor:
        Debugging().Update(input);
        if (mGridMap)
        {
            if (mWorldState.mState == WorldState::States::eInEditor)
            {
                mEditorMode->Update(input, coords);
            }
            else if (mWorldState.mState == WorldState::States::eInGame)
            {
                mGameMode->Update(input, coords);
            }
            else
            {
                if (mWorldState.mState == WorldState::States::eToEditor)
                {
                    coords.SetScreenSize(glm::vec2(coords.Width(), coords.Height()) * mEditorMode->mEditorCamZoom);
                    if (SDL_TICKS_PASSED(SDL_GetTicks(), mWorldState.mModeSwitchTimeout))
                    {
                        mWorldState.mState = WorldState::States::eInEditor;
                    }
                }
                else if (mWorldState.mState == WorldState::States::eToGame)
                {
                    coords.SetScreenSize(mWorldState.kVirtualScreenSize);
                    if (SDL_TICKS_PASSED(SDL_GetTicks(), mWorldState.mModeSwitchTimeout))
                    {
                        mWorldState.mState = WorldState::States::eInGame;
                    }
                }
                coords.SetCameraPosition(mWorldState.mCameraPosition);
            }
        }
        break;

    case WorldState::States::ePlayFmv:
        if (!mPlayFmvState->Update(input))
        {
            mWorldState.mState = mWorldState.mReturnToState;
        }
        break;

    case WorldState::States::eLoadingMap:
        if (mLocatePathFuture && FutureIsDone(mLocatePathFuture))
        {
            mPathBeingLoaded = mLocatePathFuture->get(); // Will re-throw anything that was thrown during the async processing
            mLocatePathFuture = nullptr;
        }

        if (!mLocatePathFuture)
        {
            if (mPathBeingLoaded)
            {
                // Note: This is iterative loading which happens in the main thread
                if (LoadMap(*mPathBeingLoaded))
                {
                    mWorldState.mState = WorldState::States::eSoundsLoading;
                    mSound.SetMusicTheme(mPathBeingLoaded->MusicThemeName().c_str());
                }
            }
            else
            {
                // TODO: Throw ?
                LOG_ERROR("LVL or file in LVL not found");
                mWorldState.mState = WorldState::States::eInGame;
                mLoadingIcon.SetEnabled(false);
            }
        }
        break;

    case WorldState::States::eSoundsLoading:
        // TODO: Construct sprite sheets for objects that exist in this map
        if (!mSound.IsLoading())
        {
            mWorldState.mState = WorldState::States::eInGame;
            mSound.HandleMusicEvent("BASE_LINE");
            mLoadingIcon.SetEnabled(false);
        }
        break;

    case WorldState::States::eQuit:
        return EngineStates::eQuit;

    case WorldState::States::eNone:
        break;
    }

    mSound.Update();

    return EngineStates::eRunSelectedGame;
}

void World::Render(AbstractRenderer& /*rend*/)
{
   

    switch (mWorldState.mState)
    {
    case WorldState::States::eInGame:
    case WorldState::States::eToEditor:
    case WorldState::States::eToGame:
    case WorldState::States::eInEditor:
        mRenderer.Clear(0.4f, 0.4f, 0.4f);

        Debugging().Render(mRenderer);
        mPlayFmvState->RenderDebugSubsAndFontAtlas(mRenderer);

        if (mGridMap)
        {
            if (mWorldState.mState == WorldState::States::eInEditor)
            {
                mEditorMode->Render(mRenderer);
            }
            else if (mWorldState.mState == WorldState::States::eInGame)
            {
                mGameMode->Render(mRenderer);
            }
            else
            {
                mEditorMode->Render(mRenderer);
            }
        }
        break;

    case WorldState::States::ePlayFmv:
        mPlayFmvState->Render(mRenderer);
        break;

    case WorldState::States::eQuit:
        mRenderer.Clear(0.0f, 0.0f, 0.0f);
        break;

    case WorldState::States::eLoadingMap:
        // TODO: Need to keep rendering the last camera/objects while loading
        // and then run the transition effect
        mRenderer.Clear(0.0f, 0.0f, 0.0f);
        break;

    case WorldState::States::eSoundsLoading:
        break;

    case WorldState::States::eNone:
        break;
    }
}

void World::RenderDebugPathSelection()
{
    if (ImGui::CollapsingHeader("Maps"))
    {
        for (const auto& pathMap : mLocator.PathMaps())
        {
            if (ImGui::Button(pathMap.first.c_str()))
            {
                Debugging().fnLoadPath(pathMap.first.c_str());
            }
        }
    }
}

void World::RenderDebugFmvSelection()
{
    if (mFmvDebugUi->Ui())
    {
        mPlayFmvState->Play(mFmvDebugUi->FmvName().c_str(), mFmvDebugUi->FmvFileLocation());
        mWorldState.mReturnToState = mWorldState.mState;
        mWorldState.mState = WorldState::States::ePlayFmv;
    }
}

void World::UnloadMap(AbstractRenderer& renderer)
{
    mGridMap->UnloadMap(renderer);
}
