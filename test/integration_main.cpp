#include "SDL.h"
#include "engine.hpp"
#include "logger.hpp"
#include "fmv.hpp"
#include "alive_version.h"
#include "imgui/imgui.h"
#include "sound.hpp"

extern "C"
{
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

    // Hack to make SDL2 link
    #if _MSC_VER == 1900
    void *__iob_func()
    {
        static FILE f[] = { *stdin, *stdout, *stderr };
        return f;
    }
    #endif
}

class TestEngine : public Engine
{
public:
    virtual void DebugRender() override
    {
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
        }
    }

    virtual void InitSubSystems() override
    {
        mFmv = std::make_unique<DebugFmv>(mGameData, mAudioHandler, mFileSystem);
        mSound = std::make_unique<Sound>(mGameData, mAudioHandler, mFileSystem);
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
