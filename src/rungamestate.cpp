#include "rungamestate.hpp"
#include "abstractrenderer.hpp"
#include "animationbrowser.hpp"
#include "gridmap.hpp"

RunGameState::RunGameState(ResourceLocator& locator)
    : mResourceLocator(locator), mAnimBrowser(locator)
{

}

void RunGameState::Render(AbstractRenderer& renderer)
{
    renderer.Clear(0.4f, 0.4f, 0.4f);

    mAnimBrowser.Render(renderer);
    mLevel->Render(renderer);
}

EngineStates RunGameState::Update(const InputState& input, CoordinateSpace& coords)
{   
    if (!mLevel)
    {
        //mLevel = std::make_unique<Level>(*mSound, mAudioHandler, mResourceLocator, *mRenderer);
    }

    mAnimBrowser.Update(input, coords);
    mLevel->Update(input, coords);

    return EngineStates::eRunGameState;
}
