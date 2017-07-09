#include "rungamestate.hpp"
#include "abstractrenderer.hpp"
#include "animationbrowser.hpp"
#include "gridmap.hpp"
#include "fmv.hpp"
#include "sound.hpp"

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

}


void RunGameState::OnStart(const std::string& initScriptName, Sound* pSound)
{
    mSound = pSound;

    const std::string gameScript = mResourceLocator.LocateScript(initScriptName.c_str());

    Sqrat::Script script;
    script.CompileString(gameScript, initScriptName);
    SquirrelVm::CheckError();

    script.Run();
    SquirrelVm::CheckError();

    mSound->CacheMemoryResidentSounds();
}

void RunGameState::LoadMap(const std::string& /*mapName*/)
{

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

    if (!mLevel)
    {
        mLevel = std::make_unique<Level>(*mSound, mResourceLocator, mRenderer);
    }

    if (mLevel)
    {
        mLevel->Update(input, coords);
    }

    mAnimBrowser.Update(input, coords);

    mSound->Update();

    return EngineStates::eRunGameState;
}
