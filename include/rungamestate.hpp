#pragma once

#include "animationbrowser.hpp"
#include "sound.hpp"
#include "resourcemapper.hpp"

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
    ~RunGameState();
    void Render();
    void OnStartASync(const std::string& initScriptName, Sound* pSound);
    void LoadMap(const std::string& mapName);
    EngineStates Update(const InputState& input, CoordinateSpace& coords);
    static void RegisterScriptBindings();
    bool IsLoading() const;
private:
    ResourceLocator& mResourceLocator;
    AbstractRenderer& mRenderer;
    AnimationBrowser mAnimBrowser;
    std::unique_ptr<class Level> mLevel;
    Sound* mSound = nullptr;
    enum class RunGameStates
    {
        eInit,
        eLoadingSoundEffects,
        eLoadingMap,
        eRunning,
    };
    RunGameStates mState = RunGameStates::eInit;
    Sqrat::Script mMainScript;
    up_future_void mLoadSoundEffectsFuture;
    up_future_UP_Path mLocatePathFuture;
    Oddlib::UP_Path mPathBeingLoaded;
};
