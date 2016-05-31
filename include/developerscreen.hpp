#pragma once

#include "engine.hpp"
#include "resourcemapper.hpp"

class DevloperScreen : public EngineState
{
public:
    DevloperScreen(GuiContext* gui, IEngineStateChanger& stateChanger, class Fmv& fmv, class Sound& sound, class Level& level, class FileSystem& fs, ResourceLocator& resMapper)
        : EngineState(stateChanger), 
          mGui(gui),
          mFmv(fmv),
          mSound(sound),
          mLevel(level),
          mFsOld(fs),
          mResourceLocator(resMapper)
    {

    }
    virtual void Update() override;
    virtual void Render(int w, int h, Renderer& renderer) override;
private:
    void RenderAnimationSelector(Renderer& renderer);

    struct GuiContext *mGui = nullptr;
    Fmv& mFmv;
    Sound& mSound;
    Level& mLevel;
    FileSystem& mFsOld;
    ResourceLocator& mResourceLocator;
};
