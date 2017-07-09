#pragma once

#include "animationbrowser.hpp"

class ResourceLocator;
class AbstractRenderer;
class CoordinateSpace;
class InputState;

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
