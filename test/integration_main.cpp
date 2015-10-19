#include "SDL.h"
#include "engine.hpp"
#include "logger.hpp"
#include "fmv.hpp"
#include "alive_version.h"
#include "sound.hpp"
#include "msvc_sdl_link.hpp"

class TestEngine : public Engine
{
public:
    virtual void DebugRender() override
    {
        /*
        // Render main menu bar
        static bool showAbout = false;
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("Help"))
            {
                if (ImGui::MenuItem("About", "CTRL+H"))
                {
                    showAbout = !showAbout;
                }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        //ImGui::ShowTestWindow();

        // Render about screen
        if (showAbout)
        {
            ImGui::Begin(ALIVE_VERSION_NAME_STR, nullptr, ImVec2(400, 100));
            ImGui::Text("Open source ALIVE engine PaulsApps.com 2015");
            ImGui::End();
        }*/
    }

    virtual void InitSubSystems() override
    {
        Engine::InitSubSystems();
        mFmv = std::make_unique<DebugFmv>(mGameData, mAudioHandler, mFileSystem);
    }

    virtual bool Init() override
    {
        return Engine::Init();
    }

    virtual int Run() override
    {
        return Engine::Run();
    }
};

int main(int /*argc*/, char** /*argv*/)
{
    TRACE_ENTRYEXIT;

    TestEngine e;
    if (!e.Init())
    {
        return 1;
    }


    return e.Run();
}
