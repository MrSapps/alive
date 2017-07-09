#pragma once

#include "animationbrowser.hpp"

class ResourceLocator;
class AbstractRenderer;
class CoordinateSpace;
class InputState;

class PlayFmvState
{
public:
    PlayFmvState(IAudioController& audioController, ResourceLocator& locator);
    void Render(AbstractRenderer& renderer);
    EngineStates Update(const InputState& input);
private:
    std::unique_ptr<class Fmv> mFmv;
};

class RunGameState
{
public:
    RunGameState(ResourceLocator& locator);
    void Render(AbstractRenderer& renderer);
    EngineStates Update(const InputState& input, CoordinateSpace& coords);
private:
    ResourceLocator& mResourceLocator;
    AnimationBrowser mAnimBrowser;
    std::unique_ptr<class Level> mLevel;
};
