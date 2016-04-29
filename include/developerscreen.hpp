#pragma once

#include "engine.hpp"

class DevloperScreen : public EngineState
{
public:
    DevloperScreen(GuiContext* gui, IEngineStateChanger& stateChanger, class Fmv& fmv, class Sound& sound, class Level& level, class FileSystem& fs) 
        : EngineState(stateChanger), 
          mGui(gui),
          mFmv(fmv),
          mSound(sound),
          mLevel(level),
          mFsOld(fs)
    {

    }
    virtual void Update() override;
    virtual void Render(int w, int h, Renderer& renderer) override;
private:
    struct GuiContext *mGui = nullptr;
    Fmv& mFmv;
    Sound& mSound;
    Level& mLevel;
    FileSystem& mFsOld;
};
