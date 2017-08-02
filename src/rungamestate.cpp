#include "rungamestate.hpp"
#include "abstractrenderer.hpp"
#include "animationbrowser.hpp"
#include "gridmap.hpp"
#include "fmv.hpp"
#include "sound.hpp"
#include "resourcemapper.hpp"

PlayFmvState::PlayFmvState(IAudioController& audioController, ResourceLocator& locator)
{
    mFmv = std::make_unique<Fmv>(audioController, locator);
}

void PlayFmvState::Render(AbstractRenderer& renderer)
{
    renderer.Clear(0.0f, 0.0f, 0.0f);
    mFmv->Render(renderer);
}

EngineStates PlayFmvState::Update(const InputState& input)
{
    mFmv->Update();

    if (mFmv->IsPlaying())
    {
        if (input.Mapping().GetActions().mIsPressed)
        {
            LOG_INFO("Stopping FMV due to key press");
            mFmv->Stop();
            return EngineStates::eRunGameState;
        } 
        return EngineStates::ePlayFmv;
    }
    return EngineStates::eRunGameState;
}

void PlayFmvState::Play(const char* fmvName)
{
    mFmv->Play(fmvName);
}

// ==============================================================

RunGameState::RunGameState(ResourceLocator& locator, AbstractRenderer& renderer)
    : mResourceLocator(locator), mRenderer(renderer), mAnimBrowser(locator)
{
    mLevel = std::make_unique<Level>(mResourceLocator);

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
        for (const auto& pathMap : mResourceLocator.PathMaps())
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
}

RunGameState::~RunGameState()
{
    if (mLevel)
    {
        mLevel->UnloadMap(mRenderer);
    }
}

// Engine init pending
// First init pending
// Map load pending
// Screen change pending
// Screen change across maps pending
// To editor mode pending
// To game mode pending

void RunGameState::OnStartASync(const std::string& initScriptName, Sound* pSound)
{
    mSound = pSound;

    const std::string gameScript = mResourceLocator.LocateScript(initScriptName.c_str());

    Sqrat::Script script;
    mMainScript.CompileString(gameScript, initScriptName);
    SquirrelVm::CheckError();

    // TODO: Exec via script
    mMainScript.Run();

    //LoadMap("BAPATH1");
    Debugging().mFnNextPath();
}

void RunGameState::LoadMap(const std::string& mapName)
{
    mLevel->UnloadMap(mRenderer);

    mLocatePathFuture = mResourceLocator.LocatePath(mapName.c_str());
    mState = RunGameStates::eLoadingMap;
}

bool RunGameState::IsLoading() const
{
    return mState != RunGameStates::eRunning;
}

void RunGameState::Render()
{
    mRenderer.Clear(0.4f, 0.4f, 0.4f);

    if (mLevel)
    {
        mLevel->Render(mRenderer);
    }

    mAnimBrowser.Render(mRenderer);
}

EngineStates RunGameState::Update(const InputState& input, CoordinateSpace& coords)
{   
    if (mState == RunGameStates::eRunning)
    {
        if (mLevel)
        {
            // TODO: This can change state
            mLevel->Update(input, coords);
        }
    }
    else if (mState == RunGameStates::eLoadingMap)
    {
        if (mLocatePathFuture && FutureIsDone(mLocatePathFuture))
        {
            mPathBeingLoaded = mLocatePathFuture->get(); // Will re-throw anything that was throwning during the async processing
            mLocatePathFuture = nullptr;
        }

        if (!mLocatePathFuture)
        {
            if (mPathBeingLoaded)
            {
                // Note: This is iterative loading which happens in the main thread
                if (mLevel->LoadMap(*mPathBeingLoaded))
                {
                    mState = RunGameStates::eSoundsLoading;
                    mSound->SetMusicTheme(mPathBeingLoaded->MusicThemeName().c_str());
                }
            }
            else
            {
                // TODO: Throw ?
                LOG_ERROR("LVL or file in LVL not found");
                mState = RunGameStates::eRunning;
            }
        }
    }
    else if (mState == RunGameStates::eSoundsLoading || mState == RunGameStates::eSoundsLoadingAgain)
    {
        // TODO: Construct sprite sheets for objects that exist in this map
        if (!mSound->IsLoading())
        {
            mState = RunGameStates::eRunning;
            if (mState != RunGameStates::eSoundsLoadingAgain)
            {
                // If a script explicitly changed the music theme then don't auto start playing the base line
                mSound->HandleMusicEvent("BASE_LINE");
            }
        }
    }

    mAnimBrowser.Update(input, coords);

    mSound->Update();

    if (mState == RunGameStates::eRunning && mSound->IsLoading())
    {
        // If any point mid-game something changes the sound them then go back to this state
        mState = RunGameStates::eSoundsLoading;
    }

    return EngineStates::eRunGameState;
}
