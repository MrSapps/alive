#include "developerscreen.hpp"
#include "gui.h"
#include "renderer.hpp"
#include "oddlib/anim.hpp"
#include "oddlib/lvlarchive.hpp"
#include "fmv.hpp"
#include "sound.hpp"
#include "gridmap.hpp"

struct ResInfo
{
    ResInfo() = default;
    ResInfo(const ResInfo&) = delete;
    ResInfo& operator = (const ResInfo&) = delete;

    bool mDisplay;
    Oddlib::LvlArchive::FileChunk* mFileChunk;
    std::unique_ptr<Oddlib::AnimationSet> mAnim;

    Uint32 counter = 0;
    Uint32 frameNum = 0;
    Uint32 animNum = 0;

    void Animate(Renderer& rend)
    {


        const Oddlib::Animation* anim = mAnim->AnimationAt(animNum);


        const Oddlib::Animation::Frame& frame = anim->GetFrame(frameNum);
        counter++;
        if (counter > 25)
        {
            counter = 0;
            frameNum++;
            if (frameNum >= anim->NumFrames())
            {
                frameNum = 0;
                animNum++;
                if (animNum >= mAnim->NumberOfAnimations())
                {
                    animNum = 0;
                }
            }
        }

        const int textureId = rend.createTexture(GL_RGBA, frame.mFrame->w, frame.mFrame->h, GL_RGBA, GL_UNSIGNED_BYTE, frame.mFrame->pixels, true);

        int scale = 3;
        float xpos = 300.0f + (frame.mOffX*scale);
        float ypos = 300.0f + (frame.mOffY*scale);
        // LOG_INFO("Pos " << xpos << "," << ypos);
        BlendMode blend = BlendMode::B100F100(); // TODO: Detect correct blending
        Color color = Color::white();
        rend.drawQuad(textureId, xpos, ypos, static_cast<float>(frame.mFrame->w*scale), static_cast<float>(frame.mFrame->h*scale), color, blend);

        rend.destroyTexture(textureId);
    }
};

void DevloperScreen::Update()
{
    //mFmv.Play("INGRDNT.DDV");
    mFmv.Update();
    mSound.Update();
    mLevel.Update();
}

void DevloperScreen::Render(int w, int h, Renderer& renderer)
{
    //DebugRender();

    { // Editor user interface
        // When this gets bigger it can be moved to a separate class etc.
        struct EditorUi
        {
            bool resPathsOpen;
            bool fmvBrowserOpen;
            bool soundBrowserOpen;
            bool levelBrowserOpen;
            bool animationBrowserOpen;
            bool guiLayoutEditorOpen;
        };


        static EditorUi editor;

        gui_begin_window(mGui, "Browsers");
        gui_checkbox(mGui, "resPathsOpen|Resource paths", &editor.resPathsOpen);
        gui_checkbox(mGui, "fmvBrowserOpen|FMV browser", &editor.fmvBrowserOpen);
        gui_checkbox(mGui, "soundBrowserOpen|Sound browser", &editor.soundBrowserOpen);
        gui_checkbox(mGui, "levelBrowserOpen|Level browser", &editor.levelBrowserOpen);
        gui_checkbox(mGui, "animationBrowserOpen|Animation browser", &editor.animationBrowserOpen);
        gui_checkbox(mGui, "guiLayoutEditOpen|GUI layout editor", &editor.guiLayoutEditorOpen);

        gui_end_window(mGui);

        if (editor.resPathsOpen)
        {
            mFsOld.DebugUi(*mGui);
        }

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

            static std::vector<std::pair<std::string, std::unique_ptr<ResInfo>>> resources;
            if (resources.empty())
            {
                // Add all "anim" resources to a big list
                // HACK: all leaked
                static auto stream = mFsOld.ResourcePaths().Open("BA.LVL");
                static Oddlib::LvlArchive lvlArchive(std::move(stream));
                for (auto i = 0u; i < lvlArchive.FileCount(); i++)
                {
                    Oddlib::LvlArchive::File* file = lvlArchive.FileByIndex(i);
                    for (auto j = 0u; j < file->ChunkCount(); j++)
                    {
                        Oddlib::LvlArchive::FileChunk* chunk = file->ChunkByIndex(j);
                        if (chunk->Type() == Oddlib::MakeType('A', 'n', 'i', 'm'))
                        {
                            auto info = std::make_unique<ResInfo>();
                            info->mDisplay = false;
                            info->mFileChunk = chunk;
                            resources.emplace_back(std::make_pair(file->FileName() + "_" + std::to_string(chunk->Id()), std::move(info)));
                        }
                    }
                }
            }

            gui_begin_window(mGui, "Animations");
            for (auto& res : resources)
            {
                gui_checkbox(mGui, res.first.c_str(), &res.second->mDisplay);
                if (res.second->mDisplay)
                {
                    Oddlib::LvlArchive::FileChunk* chunk = res.second->mFileChunk;
                    if (!res.second->mAnim)
                    {
                        res.second->mAnim = Oddlib::LoadAnimations(*chunk->Stream(), false);
                    }

                    renderer.beginLayer(gui_layer(mGui));
                    res.second->Animate(renderer);
                    renderer.endLayer();
                }
            }

            gui_end_window(mGui);

        }

        if (editor.guiLayoutEditorOpen)
        {
            gui_layout_editor(mGui, "../src/generated_gui_layout.cpp");
        }
    }
}
