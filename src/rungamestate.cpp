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
    script.CompileString(gameScript, initScriptName);
    SquirrelVm::CheckError();

    script.Run();
    SquirrelVm::CheckError();

    mSound->CacheMemoryResidentSounds();

    // TODO: Should be the script calling this
    Debugging().mFnNextPath();
}

void RunGameState::RegisterScriptBindings()
{
    Sqrat::Class<Sound, Sqrat::NoConstructor<Sound>> c(Sqrat::DefaultVM::Get(), "Game");
    c.Func("LoadMap", &RunGameState::LoadMap);
    Sqrat::RootTable().Bind("Game", c);
}

void RunGameState::LoadMap(const std::string& mapName)
{
    std::unique_ptr<Oddlib::Path> path = mResourceLocator.LocatePath(mapName.c_str());
    if (path)
    {
        mLevel->LoadMap(*path);

        //mSound->SetTheme(path->MusicTheme());
    }
    else
    {
        LOG_ERROR("LVL or file in LVL not found");
    }
}

void RunGameState::Render(AbstractRenderer& renderer)
{
    renderer.Clear(0.4f, 0.4f, 0.4f);

    if (mLevel)
    {
        mLevel->Render(renderer);
    }

    mAnimBrowser.Render(renderer);
}


EngineStates RunGameState::Update(const InputState& input, CoordinateSpace& coords)
{   
    // TODO: Load the game script which then loads the first level

    if (mLevel)
    {
        mLevel->Update(input, coords);
    }

    mAnimBrowser.Update(input, coords);

    mSound->Update();

    return EngineStates::eRunGameState;
}
