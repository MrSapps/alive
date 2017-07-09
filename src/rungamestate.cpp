#include "rungamestate.hpp"
#include "abstractrenderer.hpp"
#include "animationbrowser.hpp"
#include "gridmap.hpp"
#include "fmv.hpp"

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

RunGameState::RunGameState(ResourceLocator& locator)
    : mResourceLocator(locator), mAnimBrowser(locator)
{

}

void RunGameState::Render(AbstractRenderer& renderer)
{
    renderer.Clear(0.4f, 0.4f, 0.4f);

    mAnimBrowser.Render(renderer);
   // mLevel->Render(renderer);
}

EngineStates RunGameState::Update(const InputState& input, CoordinateSpace& coords)
{   
    // TODO: Load the game script which then loads the first level

    if (!mLevel)
    {
        //mLevel = std::make_unique<Level>(*mSound, mResourceLocator, *mRenderer);
    }

    mAnimBrowser.Update(input, coords);
    //mLevel->Update(input, coords);

    return EngineStates::eRunGameState;
}
