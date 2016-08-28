#include "engine.hpp"
#include <iostream>
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
#include "gameselectionscreen.hpp"
#include "generated_gui_layout.cpp" // Has function "load_layout" to set gui layout. Only used in single .cpp file.


#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include "rsc\resource.h"
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
#ifdef _MSC_VER
#pragma warning(push)
            // C4191: 'reinterpret_cast' : unsafe conversion from 'FARPROC' to 'pSetConsoleIcon'
            // This is a "feature" of GetProcAddress, so ignore.
#pragma warning(disable:4191)
#endif
            pSetConsoleIcon setConsoleIcon = reinterpret_cast<pSetConsoleIcon>(::GetProcAddress(hKernel32, "SetConsoleIcon"));
#ifdef _MSC_VER
#pragma warning(pop)
#endif
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
        // load the list of data paths (if any) and discover what they are
        mFileSystem = std::make_unique<GameFileSystem>();
        if (!mFileSystem->Init())
        {
            LOG_ERROR("File system init failure");
            return false;
        }

        InitResources();

        if (!InitSDL())
        {
            LOG_ERROR("SDL init failure");
            return false;
        }

        InitGL();

        InitSubSystems();

        
        mStateMachine.ToState(std::make_unique<GameSelectionScreen>(mStateMachine, mGameDefinitions, mGui, *mFmv, *mSound, *mLevel, *mResourceLocator, *mFileSystem));

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
    mRenderer = std::make_unique<Renderer>((mFileSystem->FsPath() + "data/Roboto-Regular.ttf").c_str());
    mFmv = std::make_unique<DebugFmv>(mAudioHandler, *mResourceLocator);
    mSound = std::make_unique<Sound>(mAudioHandler, *mResourceLocator);
    mLevel = std::make_unique<Level>(mAudioHandler, *mResourceLocator);

    { // Init gui system
        mGui = create_gui(&calcTextSize, mRenderer.get());
        load_layout(mGui);
    }
}

// TODO: Using averaging value or anything that is more accurate than this
typedef std::chrono::high_resolution_clock THighResClock;
class BasicFramesPerSecondCounter
{
public:
    BasicFramesPerSecondCounter()
    {
        mStartedTime = THighResClock::now();
    }

    template<class T>
    void Update(T fnUpdate)
    {
        mFramesPassed++;
        const THighResClock::duration totalRunTime = THighResClock::now() - mStartedTime;
        const Sint64 timeSinceLastFrameInMsecs = std::chrono::duration_cast<std::chrono::milliseconds>(totalRunTime - mFrameStartTime).count();
        if (timeSinceLastFrameInMsecs > 500 && mFramesPassed > 10)
        {
            const f32 fps = static_cast<f32>(static_cast<f32>(mFramesPassed) / (static_cast<f32>(timeSinceLastFrameInMsecs) / 1000.0f));
            mFrameStartTime = totalRunTime;
            mFramesPassed = 0;
            fnUpdate(fps);
        }
    }

private:
    THighResClock::time_point mStartedTime = {};
    u32 mFramesPassed = 0;
    THighResClock::duration mFrameStartTime = {};
};

static char* WindowTitle(f32 fps)
{
    static char buffer[128] = {};
    sprintf(buffer, ALIVE_VERSION_NAME_STR, fps);
    return buffer;
}

int Engine::Run()
{
    BasicFramesPerSecondCounter fpsCounter;
    auto startTime = THighResClock::now();

    while (mStateMachine.HasState())
    {
        // Limit update to 60fps
        const THighResClock::duration totalRunTime = THighResClock::now() - startTime;
        const Sint64 timePassed = std::chrono::duration_cast<std::chrono::nanoseconds>(totalRunTime).count();
        if (timePassed >= 16666666)
        {
            Update();
            startTime = THighResClock::now();
        }

        Render();
        fpsCounter.Update([&](f32 fps)
        {
            SDL_SetWindowTitle(mWindow, WindowTitle(fps));
        });
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

    // TODO: Map "player" input to "game" buttons
    if (mInputState.mLeftMouseState & InputState::eDown)
    {
        mInputState.mLeftMouseState |= InputState::eHeld;
    }
    else if (mInputState.mLeftMouseState & InputState::eUp)
    {
        mInputState.mLeftMouseState = 0;
    }

    if (mInputState.mRightMouseState & InputState::eDown)
    {
        mInputState.mRightMouseState |= InputState::eHeld;
    }
    else if (mInputState.mRightMouseState & InputState::eUp)
    {
        mInputState.mRightMouseState = 0;
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
            mStateMachine.ToState(nullptr);
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
            if (event.button.button == SDL_BUTTON_LEFT)
            {
                guiKey = GUI_KEY_LMB;

                if (event.type == SDL_MOUSEBUTTONUP)
                {
                    mInputState.mLeftMouseState = InputState::eUp;
                }
                else
                {
                    mInputState.mLeftMouseState = InputState::eDown;
                }
            }
            else if (event.button.button == SDL_BUTTON_MIDDLE)
            {
                guiKey = GUI_KEY_MMB;
            }
            else if (event.button.button == SDL_BUTTON_RIGHT)
            {
                guiKey = GUI_KEY_RMB;

                if (event.type == SDL_MOUSEBUTTONUP)
                {
                    mInputState.mRightMouseState = InputState::eUp;
                }
                else
                {
                    mInputState.mRightMouseState = InputState::eDown;
                }
            }

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
                const u32 windowFlags = SDL_GetWindowFlags(mWindow);
                bool isFullScreen = ((windowFlags & SDL_WINDOW_FULLSCREEN_DESKTOP) || (windowFlags & SDL_WINDOW_FULLSCREEN));
                SDL_SetWindowFullscreen(mWindow, isFullScreen ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
                //OnWindowResize();
            }


            const SDL_Scancode key = SDL_GetScancodeFromKey(event.key.keysym.sym);

            // TODO: Move out of here
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

        mInputState.mMouseX = mouse_x;
        mInputState.mMouseY = mouse_y;

        if (SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_LEFT))
        {
            mGui->key_state[GUI_KEY_LMB] |= GUI_KEYSTATE_DOWN_BIT;
        }


        if (SDL_GetKeyboardState(NULL)[SDL_SCANCODE_LCTRL])
        {
            mGui->key_state[GUI_KEY_LCTRL] |= GUI_KEYSTATE_DOWN_BIT;
        }
    }

    mStateMachine.Input(mInputState);
    mStateMachine.Update();
}

void Engine::Render()
{
    int w = 0;
    int h = 0;
    SDL_GetWindowSize(mWindow, &w, &h);
    mGui->host_win_size[0] = w;
    mGui->host_win_size[1] = h;

    mRenderer->beginFrame(w, h);
    gui_pre_frame(mGui); 

    mStateMachine.Render(w, h, *mRenderer);

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

    mWindow = SDL_CreateWindow(WindowTitle(0.0f),
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640*2, 480*2,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!mWindow)
    {
        LOG_ERROR("Failed to create window: " << SDL_GetError());
        return false;
    }

    SDL_SetWindowMinimumSize(mWindow, 320, 240);

#if defined(_WIN32)
    // I'd like my icon back thanks
    setWindowsIcon(mWindow);
#endif

    return true;
}

void Engine::AddGameDefinitionsFrom(const char* path)
{
    const auto jsonFiles = mFileSystem->EnumerateFiles(path, "*.json");
    for (const auto& gameDef : jsonFiles)
    {
        mGameDefinitions.emplace_back(*mFileSystem, (std::string(path) + "/" + gameDef).c_str(), false);
    }
}

void Engine::AddModDefinitionsFrom(const char* path)
{
    std::string strPath(path);
    AddDirectoryBasedModDefinitionsFrom(strPath);
    AddZipsedModDefinitionsFrom(strPath);
}

void Engine::AddDirectoryBasedModDefinitionsFrom(std::string path)
{
    const auto possibleModDirs = mFileSystem->EnumerateFolders(path);
    for (const auto& possibleModDir : possibleModDirs)
    {
        auto modDefinitionFiles = mFileSystem->EnumerateFiles(path + "/" + possibleModDir, "game.json");
        if (!modDefinitionFiles.empty())
        {
            auto fs = IFileSystem::Factory(*mFileSystem, path + "/" + possibleModDir + "/");
            if (fs)
            {
                mGameDefinitions.emplace_back(*fs, modDefinitionFiles[0].c_str(), true);
            }
        }
    }
}

void Engine::AddZipsedModDefinitionsFrom(std::string path)
{
    const auto possibleModZips = mFileSystem->EnumerateFiles(path, "*.zip");
    for (const auto& possibleModZip : possibleModZips)
    {
        auto fs = IFileSystem::Factory(*mFileSystem, path + "/" + possibleModZip);
        if (fs)
        {
            auto modDefinitionFiles = fs->EnumerateFiles("", "game.json");
            if (!modDefinitionFiles.empty())
            {
                mGameDefinitions.emplace_back(*fs, modDefinitionFiles[0].c_str(), true);
            }
        }
    }
}

void Engine::InitResources()
{
    TRACE_ENTRYEXIT;

    // load the enumerated "built in" game defs
    AddGameDefinitionsFrom("{GameDir}/data/GameDefinitions");

    // load the enumerated "mod" game defs
    AddModDefinitionsFrom("{UserDir}/Mods");

    // The engine probably won't ship with any mods, but while under development look here too
    AddModDefinitionsFrom("{GameDir}/data/Mods");

    // create the resource mapper loading the resource maps from the json db
    DataPaths dataPaths(*mFileSystem, "{GameDir}/data/DataSetIds.json", "{UserDir}/DataSets.json");
    ResourceMapper mapper(*mFileSystem, "{GameDir}/data/resources.json");
    mResourceLocator = std::make_unique<ResourceLocator>(std::move(mapper), std::move(dataPaths));

    // TODO: After user selects game def then add/validate the required paths/data sets in the res mapper
    // also add in any extra maps for resources defined by the mod @ game selection screen
}

void Engine::InitGL()
{
    // SDL Defaults
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 3);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 3);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 2);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 0);

    // Overrides
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);

    mContext = SDL_GL_CreateContext(mWindow);
    if (!mContext)
    {
        throw Oddlib::Exception((std::string("Failed to create GL context: ") + SDL_GetError()).c_str());
    }

    int r = 0;
    int g = 0;
    int b = 0;
    int a = 0;
    int bufferSize = 0;
    int doubleBuffer = 0;
    SDL_GL_GetAttribute(SDL_GL_RED_SIZE, &r);
    SDL_GL_GetAttribute(SDL_GL_GREEN_SIZE, &g);
    SDL_GL_GetAttribute(SDL_GL_BLUE_SIZE, &b);
    SDL_GL_GetAttribute(SDL_GL_ALPHA_SIZE, &a);
    SDL_GL_GetAttribute(SDL_GL_BUFFER_SIZE, &bufferSize);
    SDL_GL_GetAttribute(SDL_GL_DOUBLEBUFFER, &doubleBuffer);
    LOG_INFO("GL settings r " << r << " g " << g << " b " << b << " bufferSize " << bufferSize << " double buffer " << doubleBuffer);

    SDL_GL_SetSwapInterval(0); // No vsync for gui, for responsiveness

    if (gl3wInit()) 
    {
        throw Oddlib::Exception("failed to initialize OpenGL");
    }

    if (!gl3wIsSupported(3, 1)) 
    {
        throw Oddlib::Exception("OpenGL 3.1 not supported");
    }
}
