#pragma once

#include "engine.hpp"
#include "resourcemapper.hpp"

class DeveloperScreen : public IState
{
public:
    DeveloperScreen(DeveloperScreen&&) = delete;
    DeveloperScreen& operator = (DeveloperScreen&&) = delete;
    DeveloperScreen(StateMachine& stateMachine, GuiContext* gui, class Fmv& fmv, class Sound& sound, class Level& level, ResourceLocator& resMapper)
        : IState(stateMachine),
          mGui(gui),
          mFmv(fmv),
          mSound(sound),
          mLevel(level),
          mResourceLocator(resMapper)
    {
        Init();
    }

    virtual void Render(int w, int h, Renderer& renderer) override;
    virtual void Update(const InputState& input) override;
    virtual void EnterState() override;
    virtual void ExitState() override;
private:
    void Init();
    void RenderAnimationSelector(Renderer& renderer);

    struct GuiContext *mGui = nullptr;
    Fmv& mFmv;
    Sound& mSound;
    Level& mLevel;

    ResourceLocator& mResourceLocator;
    std::vector<std::unique_ptr<Animation>> mLoadedAnims;

    // Not owned
    Animation* mSelected = nullptr;

    s32 mXDelta = 0;
    s32 mYDelta = 0;
};
