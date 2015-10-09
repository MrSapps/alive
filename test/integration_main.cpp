#include "SDL.h"
#include "engine.hpp"
#include "logger.hpp"
#include "fmv.hpp"

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
            /*
            if (ImGui::BeginMenu("File"))
            {
            if (ImGui::MenuItem("Exit", "CTRL+Q")) { ToState(eShuttingDown); }
            ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Edit"))
            {
            if (ImGui::MenuItem("Undo", "CTRL+Z")) {}
            if (ImGui::MenuItem("Redo", "CTRL+Y", false, false)) {}  // Disabled item
            ImGui::Separator();
            if (ImGui::MenuItem("Cut", "CTRL+X")) {}
            if (ImGui::MenuItem("Copy", "CTRL+C")) {}
            if (ImGui::MenuItem("Paste", "CTRL+V")) {}
            ImGui::EndMenu();
            }*/
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
