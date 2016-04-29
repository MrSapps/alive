#include "gameselectionscreen.hpp"
#include "developerscreen.hpp"
#include "gui.h"

void GameSelectionScreen::Update()
{

}

void GameSelectionScreen::Render(int /*w*/, int /*h*/, Renderer& /*renderer*/)
{
    bool change = false;
    struct uiData
    {
        bool test;
    };
    static uiData data;

    gui_begin_window(mGui, "Select game");

    if (gui_button(mGui, "Test"))
    {
        change = true;
    }

    gui_end_window(mGui);
    if (change)
    {
        // "this" will be deleted after this call, so must be last call in the function

        mStateChanger.ToState(std::make_unique<DevloperScreen>(mGui, mStateChanger, mFmv, mSound, mLevel, mFsOld));
    }
}
