#include "developerscreen.hpp"
#include "gui.h"
#include "renderer.hpp"
#include "oddlib/anim.hpp"
#include "oddlib/lvlarchive.hpp"
#include "fmv.hpp"
#include "sound.hpp"
#include "gridmap.hpp"

void DeveloperScreen::Init()
{
    
}

void DeveloperScreen::Input(InputState& input)
{
    // Set the "selected" animation
    if (!mSelected && input.mLeftMouseState & InputState::eDown)
    {
        LOG_INFO("Get selected");
        for (auto& anim : mLoadedAnims)
        {
            if (anim->Collision(input.mMouseX, input.mMouseY))
            {
                mSelected = anim.get();

                mXDelta = std::abs(mSelected->XPos() - input.mMouseX);
                mYDelta = std::abs(mSelected->YPos() - input.mMouseY);

                LOG_INFO("Got selected");
                return;
            }
        }
    }
    
    // Move the "selected" animation
    if (mSelected && input.mLeftMouseState & InputState::eHeld)
    {
        LOG_INFO("Move selected to " << input.mMouseX << "," << input.mMouseY);
        mSelected->SetXPos(mXDelta + input.mMouseX);
        mSelected->SetYPos(mYDelta + input.mMouseY);
    }

    if (input.mLeftMouseState & InputState::eUp)
    {
        LOG_INFO("Release");
        mSelected = nullptr;
    }

    // When the right button is released delete the "selected" animation
    if (input.mRightMouseState & InputState::eUp)
    {
        LOG_INFO("Delete selected");

        for (auto it = mLoadedAnims.begin(); it != mLoadedAnims.end(); it++)
        {
            if ((*it)->Collision(input.mMouseX, input.mMouseY))
            {
                mLoadedAnims.erase(it);
                break;
            }
        }
        mSelected = nullptr;
    }
}

void DeveloperScreen::Update()
{
    //mFmv.Play("INGRDNT.DDV");
    mFmv.Update();
    mSound.Update();
    mLevel.Update();

    for (auto& anim : mLoadedAnims)
    {
        anim->Update();
    }
}

void DeveloperScreen::EnterState()
{

}

void DeveloperScreen::ExitState()
{

}

void DeveloperScreen::Render(int w, int h, Renderer& renderer)
{
    //DebugRender();

    // When this gets bigger it can be moved to a separate class etc.
    struct EditorUi
    {
        EditorUi()
            : soundBrowserOpen(true)
        {

        }
        bool fmvBrowserOpen;
        bool soundBrowserOpen;
        bool levelBrowserOpen;
        bool animationBrowserOpen;
        bool guiLayoutEditorOpen;
    };

    static EditorUi editor;

    gui_begin_window(mGui, "Browsers");
    gui_checkbox(mGui, "fmvBrowserOpen|FMV browser", &editor.fmvBrowserOpen);
    gui_checkbox(mGui, "soundBrowserOpen|Sound browser", &editor.soundBrowserOpen);
    gui_checkbox(mGui, "levelBrowserOpen|Level browser", &editor.levelBrowserOpen);
    gui_checkbox(mGui, "animationBrowserOpen|Animation browser", &editor.animationBrowserOpen);
    gui_checkbox(mGui, "guiLayoutEditOpen|GUI layout editor", &editor.guiLayoutEditorOpen);

    gui_end_window(mGui);

    if (editor.fmvBrowserOpen)
    {
        mFmv.Render(renderer, *mGui, w, h);
    }

    if (editor.soundBrowserOpen)
    {
        mSound.Render(mGui, w, h);
    }

    if (editor.levelBrowserOpen)
    {
        mLevel.Render(renderer, *mGui, w, h);
    }

    if (editor.animationBrowserOpen)
    {
        RenderAnimationSelector(renderer);
    }

    if (editor.guiLayoutEditorOpen)
    {
        gui_layout_editor(mGui, "../src/generated_gui_layout.cpp");
    }
}

void DeveloperScreen::RenderAnimationSelector(Renderer& renderer)
{
    gui_begin_window(mGui, "Animations");

    bool resetStates = false;
    if (gui_button(mGui, "Reset states"))
    {
        resetStates = true;
    }

    if (gui_button(mGui, "Clear all"))
    {
        mLoadedAnims.clear();
    }

    static char filterString[64] = {};
    gui_textfield(mGui, "Filter", filterString, sizeof(filterString));


    renderer.beginLayer(gui_layer(mGui));
    s32 spacer = 0;
    for (auto& anim : mLoadedAnims)
    {
        if (resetStates)
        {
            anim->Restart();
        }
        //anim->SetXPos(70 + spacer);
        anim->Render(renderer);
        spacer += (anim->MaxW() + (anim->MaxW()/3));
    }
    renderer.endLayer();

    auto animsToLoad = mResourceLocator.DebugUi(renderer, mGui, filterString);
    for (const auto& res : animsToLoad)
    {
        if (std::get<0>(res))
        {
            const char* dataSetName = std::get<0>(res);
            const char* resourceName = std::get<1>(res);
            bool load = std::get<2>(res);
            if (load)
            {
                auto anim = mResourceLocator.LocateAnimation(resourceName, dataSetName);
                if (anim)
                {
                    mLoadedAnims.push_back(std::move(anim));
                }
            }
        }
    }
    gui_end_window(mGui);
}
