#include "engine.hpp"
#include <iostream>
#include "oddlib/lvlarchive.hpp"
#include "oddlib/masher.hpp"
#include "oddlib/exceptions.hpp"
#include "logger.hpp"
#include "jsonxx/jsonxx.h"
#include <fstream>
#include "alive_version.h"
#include "core/audiobuffer.hpp"
#include "fmv.hpp"
#include "sound.hpp"
#include "gridmap.hpp"
#include "renderer.hpp"
#include "gui.h"
#include "guiwidgets.hpp"
#include "oddlib/anim.hpp"
#include "resourcemapper.hpp"

#include "generated_gui_layout.cpp" // Has function "load_layout" to set gui layout. Only used in single .cpp file.

extern "C"
{
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}



#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include "../rsc/resource.h"
#include "SDL_syswm.h"
#include "stdthread.h"


void setWindowsIcon(SDL_Window *sdlWindow)
{
    HINSTANCE handle = ::GetModuleHandle(nullptr);
    HICON icon = ::LoadIcon(handle, MAKEINTRESOURCE(IDI_MAIN_ICON));
    if (icon != nullptr)
    {
        SDL_SysWMinfo wminfo;
        SDL_VERSION(&wminfo.version);
        if (SDL_GetWindowWMInfo(sdlWindow, &wminfo) == 1)
        {
            HWND hwnd = wminfo.info.win.window;
#ifdef _WIN64
            ::SetClassLongPtr(hwnd, GCLP_HICON, reinterpret_cast<LONG_PTR>(icon));
#else
            ::SetClassLong(hwnd, GCL_HICON, reinterpret_cast<LONG>(icon));
#endif
        }

        HMODULE hKernel32 = ::GetModuleHandle("Kernel32.dll");
        if (hKernel32)
        {
            typedef BOOL(WINAPI *pSetConsoleIcon)(HICON icon);
#pragma warning(push)
            // C4191: 'reinterpret_cast' : unsafe conversion from 'FARPROC' to 'pSetConsoleIcon'
            // This is a "feature" of GetProcAddress, so ignore.
#pragma warning(disable:4191)
            pSetConsoleIcon setConsoleIcon = reinterpret_cast<pSetConsoleIcon>(::GetProcAddress(hKernel32, "SetConsoleIcon"));
#pragma warning(pop)
            if (setConsoleIcon)
            {
                setConsoleIcon(icon);
            }
        }
    }
}
#endif

Engine::Engine()
{

}

Engine::~Engine()
{
    destroy_gui(mGui);

    mFmv.reset();
    mSound.reset();
    mLevel.reset();
    mRenderer.reset();

    SDL_GL_DeleteContext(mContext);
    SDL_DestroyWindow(mWindow);
    SDL_Quit();
}

bool Engine::Init()
{
    try
    {
  
        // load the enumerated "built in" game defs
        GameDefinition gd;

        // load the enumerated "mod" game defs

        // load the list of data paths (if any) and discover what they are

        FileSystem2 fs;
        ResourceMapper mapper(fs, "F:\\Data\\alive\\alive\\data\\resources.json");

        // create the resource mapper loading the resource maps from the json db
        mResourceLocator = std::make_unique<ResourceLocator>(fs, gd, std::move(mapper));

        auto res = mResourceLocator->Locate<Animation>("ABEBSIC.BAN_10_31");
        res.Reload();


        // TODO: After user selects game def then add/validate the required paths/data sets in the res mapper
        // also add in any extra maps for resources defined by the mod

        if (!mFileSystem.Init())
        {
            LOG_ERROR("File system init failure");
            return false;
        }

        if (!mGameData.Init(mFileSystem))
        {
            LOG_ERROR("Game data init failure");
            return false;
        }

        if (!InitSDL())
        {
            LOG_ERROR("SDL init failure");
            return false;
        }

        InitGL();


        ToState(eRunning);

        InitSubSystems();

        return true;
    }
    catch (const std::exception& ex)
    {
        LOG_ERROR("Caught error when trying to init: " << ex.what());
        return false;
    }
}

void Engine::InitSubSystems()
{
    mRenderer = std::make_unique<Renderer>((mFileSystem.GameData().BasePath() + "/data/Roboto-Regular.ttf").c_str());
    mFmv = std::make_unique<DebugFmv>(mGameData, mAudioHandler, mFileSystem);
    mSound = std::make_unique<Sound>(mGameData, mAudioHandler, mFileSystem);
    mLevel = std::make_unique<Level>(mGameData, mAudioHandler, mFileSystem);

    { // Init gui system
        mGui = create_gui(&calcTextSize, mRenderer.get());
        load_layout(mGui);
    }
}

int Engine::Run()
{
    while (mState != eShuttingDown)
    {
        Update();
        Render();
    }
    return 0;
}

void Engine::Update()
{
    { // Reset gui input
        for (int i = 0; i < GUI_KEY_COUNT; ++i)
            mGui->key_state[i] = 0;
        mGui->cursor_pos[0] = -1;
        mGui->cursor_pos[0] = -1;
    }
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_MOUSEWHEEL:
            if (event.wheel.y < 0)
            {
                mGui->mouse_scroll = -1;
            }
            else
            {
                mGui->mouse_scroll = 1;
            }
            break;

        case SDL_QUIT:
            ToState(eShuttingDown);
            break;

        case SDL_TEXTINPUT:
        {
            size_t len = strlen(event.text.text);
            for (size_t i = 0; i < len; i++)
            {
                uint32_t keycode = event.text.text[i];
                if (keycode >= 10 && keycode <= 255)
                {
                    //printable ASCII characters
                    gui_write_char(mGui, (char)keycode);
                }
            }
        }
        break;

        case SDL_WINDOWEVENT:
            switch (event.window.event)
            {
            case SDL_WINDOWEVENT_RESIZED:
            case SDL_WINDOWEVENT_MAXIMIZED:
            case SDL_WINDOWEVENT_RESTORED:

                break;
            }
            break;
        case SDL_MOUSEBUTTONUP:
        case SDL_MOUSEBUTTONDOWN:
        {
            int guiKey = -1;
            if (event.button.button == SDL_BUTTON(SDL_BUTTON_LEFT))
                guiKey = GUI_KEY_LMB;
            else if (event.button.button == SDL_BUTTON(SDL_BUTTON_MIDDLE))
                guiKey = GUI_KEY_MMB; 
            else if (event.button.button == SDL_BUTTON(SDL_BUTTON_RIGHT))
                guiKey = GUI_KEY_RMB;

            if (guiKey >= 0)
            {
                  uint8_t state = mGui->key_state[guiKey];
                  if (event.type == SDL_MOUSEBUTTONUP)
                  {
                      state = GUI_KEYSTATE_RELEASED_BIT;
                  }
                  else
                  {
                      state = GUI_KEYSTATE_DOWN_BIT | GUI_KEYSTATE_PRESSED_BIT;
                  }
                  mGui->key_state[guiKey] = state;
            }

            // TODO: Enable SDL_CaptureMouse when sdl supports it.
            if (event.type == SDL_MOUSEBUTTONDOWN)
            {
                //SDL_CaptureMouse(SDL_TRUE);
            }
            else
            {
                //SDL_CaptureMouse(SDL_FALSE);
            }

        } break;
        case SDL_KEYDOWN:
        case SDL_KEYUP:
        {
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == 13)
            {
                const Uint32 windowFlags = SDL_GetWindowFlags(mWindow);
                bool isFullScreen = ((windowFlags & SDL_WINDOW_FULLSCREEN_DESKTOP) || (windowFlags & SDL_WINDOW_FULLSCREEN));
                SDL_SetWindowFullscreen(mWindow, isFullScreen ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
                //OnWindowResize();
            }


            const SDL_Scancode key = SDL_GetScancodeFromKey(event.key.keysym.sym);
            if (key == SDL_SCANCODE_ESCAPE)
            {
                mFmv->Stop();
            }
            if (event.type == SDL_KEYDOWN && key == SDL_SCANCODE_BACKSPACE)
                gui_write_char(mGui, '\b'); // Note that this is called in case of repeated backspace key also

            if (key == SDL_SCANCODE_LCTRL)
            {
                if (event.type == SDL_KEYDOWN)
                    mGui->key_state[GUI_KEY_LCTRL] |= GUI_KEYSTATE_PRESSED_BIT;
                else
                    mGui->key_state[GUI_KEY_LCTRL] |= GUI_KEYSTATE_RELEASED_BIT;
            }

            //SDL_Keymod modstate = SDL_GetModState();

            break;
        }
        }
    }

    { // Set rest of gui input state which wasn't set in event polling loop
        SDL_PumpEvents();

        int mouse_x, mouse_y;
        SDL_GetMouseState(&mouse_x, &mouse_y);
        mGui->cursor_pos[0] = mouse_x;
        mGui->cursor_pos[1] = mouse_y;

        if (SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_LEFT))
            mGui->key_state[GUI_KEY_LMB] |= GUI_KEYSTATE_DOWN_BIT;

        if (SDL_GetKeyboardState(NULL)[SDL_SCANCODE_LCTRL])
            mGui->key_state[GUI_KEY_LCTRL] |= GUI_KEYSTATE_DOWN_BIT;
    }

    // TODO: Move into state machine
    //mFmv.Play("INGRDNT.DDV");
    mFmv->Update();
    mSound->Update();
    mLevel->Update();
}

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
        rend.drawQuad(textureId, xpos, ypos, static_cast<float>(frame.mFrame->w*scale ), static_cast<float>(frame.mFrame->h*scale), color, blend);

        rend.destroyTexture(textureId);
    }
};

void Engine::Render()
{
    int w, h;
    SDL_GetWindowSize(mWindow, &w, &h);
    mGui->host_win_size[0] = w;
    mGui->host_win_size[1] = h;

    mRenderer->beginFrame(w, h);
    gui_pre_frame(mGui); 

    DebugRender();

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
            mFileSystem.DebugUi(*mGui);
        }

        if (editor.fmvBrowserOpen)
        {
            mFmv->Render(*mRenderer, *mGui, w, h);
        }

        if (editor.soundBrowserOpen)
        {
            mSound->Render(mGui, w, h);
        }


        if (editor.levelBrowserOpen)
        {
            mLevel->Render(*mRenderer, *mGui, w, h);
        }

        if (editor.animationBrowserOpen)
        {

            static std::vector<std::pair<std::string, std::unique_ptr<ResInfo>>> resources;
            if (resources.empty())
            {
                // Add all "anim" resources to a big list
                // HACK: all leaked
                static auto stream = mFileSystem.ResourcePaths().Open("BA.LVL");
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

                    mRenderer->beginLayer(gui_layer(mGui));
                    res.second->Animate(*mRenderer);
                    mRenderer->endLayer();
                }
            }
  
            gui_end_window(mGui);

        }

        if (editor.guiLayoutEditorOpen)
        {
            gui_layout_editor(mGui, "../src/generated_gui_layout.cpp");
        }
    }

    gui_post_frame(mGui);

    drawWidgets(*mGui, *mRenderer);
    mRenderer->endFrame();

    SDL_GL_SwapWindow(mWindow);
}

bool Engine::InitSDL()
{
    if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_EVENTS) != 0)
    {
        LOG_ERROR("SDL_Init failed " << SDL_GetError());
        return false;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    mWindow = SDL_CreateWindow(ALIVE_VERSION_NAME_STR,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640*2, 480*2,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    SDL_SetWindowMinimumSize(mWindow, 320, 240);

#if defined(_WIN32)
    // I'd like my icon back thanks
    setWindowsIcon(mWindow);
#endif

    return true;
}

void Engine::InitGL()
{
    
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 32);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);


    mContext = SDL_GL_CreateContext(mWindow);
    SDL_GL_SetSwapInterval(0); // No vsync for gui, for responsiveness

    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err == GLEW_OK)
    {
        // GLEW generates GL error because it calls glGetString(GL_EXTENSIONS), we'll consume it here.
        glGetError();

        glEnable(GL_STENCIL_TEST);
    }
    else
    {
        LOG_INFO("glewInit failure");
        throw Oddlib::Exception(reinterpret_cast<const char*>(glewGetErrorString(err)));
    }
}

void Engine::ToState(Engine::eStates newState)
{
    if (newState != mState)
    {
        mPreviousState = mState;
        mState = newState;
    }
}

