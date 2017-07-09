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
    RunGameState(ResourceLocator& locator, AbstractRenderer& renderer);
    void Render(AbstractRenderer& renderer);
    void OnStart(const std::string& initScriptName, Sound* pSound);
    void LoadMap(const std::string& mapName);
    EngineStates Update(const InputState& input, CoordinateSpace& coords);
private:
    ResourceLocator& mResourceLocator;
    AbstractRenderer& mRenderer;
    AnimationBrowser mAnimBrowser;
    std::unique_ptr<class Level> mLevel;
    Sound* mSound = nullptr;
};
