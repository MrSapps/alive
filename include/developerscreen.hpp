#pragma once

#include "engine.hpp"
#include "resourcemapper.hpp"

class DeveloperScreen : public IState
{
public:
    DeveloperScreen(DeveloperScreen&&) = delete;
    DeveloperScreen& operator = (DeveloperScreen&&) = delete;
    DeveloperScreen(StateMachine& stateMachine, class Sound& sound, class Level& level, ResourceLocator& resMapper)
        : IState(stateMachine),
          mSound(sound),
          mLevel(level),
          mResourceLocator(resMapper)
    {
        Init();
    }

    virtual void Render(int, int, AbstractRenderer& renderer) override;
    virtual void Update(const InputState& input, CoordinateSpace& coords) override;
    virtual void EnterState() override;
    virtual void ExitState() override;
private:
    void Init();
    void RenderAnimationSelector(CoordinateSpace& coords);

    Sound& mSound;
    Level& mLevel;

    ResourceLocator& mResourceLocator;
    std::vector<std::unique_ptr<Animation>> mLoadedAnims;

    // Not owned
    Animation* mSelected = nullptr;

    s32 mXDelta = 0;
    s32 mYDelta = 0;

    bool mDebugResetAnimStates = false;
};
