#pragma once

#include "engine.hpp"
#include "resourcemapper.hpp"

class DeveloperScreen : public EngineState
{
public:
    DeveloperScreen(GuiContext* gui, IEngineStateChanger& stateChanger, class Fmv& fmv, class Sound& sound, class Level& level, ResourceLocator& resMapper)
        : EngineState(stateChanger), 
          mGui(gui),
          mFmv(fmv),
          mSound(sound),
          mLevel(level),
          mResourceLocator(resMapper)
    {
        Init();
    }

    virtual void Input(InputState& input) override;
    virtual void Update() override;
    virtual void Render(int w, int h, Renderer& renderer) override;
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
