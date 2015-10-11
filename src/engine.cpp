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


extern "C"
{
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}



#ifdef _WIN32
#include <windows.h>
#include "../rsc/resource.h"
#include "SDL_syswm.h"
#include <functional>


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
            pSetConsoleIcon setConsoleIcon = (pSetConsoleIcon)::GetProcAddress(hKernel32, "SetConsoleIcon");
            if (setConsoleIcon)
            {
                setConsoleIcon(icon);
            }
        }
    }
}
#endif

static bool mousePressed[4] = { false, false };

void Engine::ImGui_WindowResize()
{
    int w, h;
    int fb_w, fb_h;
    SDL_GetWindowSize(mWindow, &w, &h);
    SDL_GetWindowSize(mWindow, &fb_w, &fb_h); // Needs to be corrected for SDL Framebuffer

    //ImGuiIO& io = ImGui::GetIO();
    //io.DisplaySize = ImVec2((float)fb_w, (float)fb_h);  // Display size, in pixels. For clamping windows positions.

    //std::cout << "ON RESIZE " << io.DisplaySize.x << " " << io.DisplaySize.y << std::endl;


    //    io.PixelCenterOffset = 0.0f;                        // Align OpenGL texels

}

void UpdateImGui()
{
    //ImGuiIO& io = ImGui::GetIO();

    SDL_PumpEvents();

    // Setup inputs
    // (we already got mouse wheel, keyboard keys & characters from glfw callbacks polled in glfwPollEvents())
    int mouse_x, mouse_y;
    SDL_GetMouseState(&mouse_x, &mouse_y);

    //io.MousePos = ImVec2((float)mouse_x * mousePosScale.x, (float)mouse_y * mousePosScale.y);      // Mouse position, in pixels (set to -1,-1 if no mouse / on another screen, etc.)

    //io.MouseDown[0] = mousePressed[0] || (SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_LEFT));
    //io.MouseDown[1] = mousePressed[1] || (SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_RIGHT));

    // Start the frame
    //ImGui::NewFrame();
}

Engine::Engine()
{

}

Engine::~Engine()
{
    mRenderer.reset();

    SDL_GL_DeleteContext(mContext);
    SDL_DestroyWindow(mWindow);
    SDL_Quit();
}

bool Engine::Init()
{
    try
    {
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
    mRenderer = std::make_unique<Renderer>((mFileSystem.BasePath() + "/data/Roboto-Regular.ttf").c_str());
    mFmv = std::make_unique<Fmv>(mGameData, mAudioHandler, mFileSystem);
    mSound = std::make_unique<Sound>(mGameData, mAudioHandler, mFileSystem);
    mLevel = std::make_unique<Level>(mGameData, mAudioHandler, mFileSystem);
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
    //ImGuiIO& io = ImGui::GetIO();
    //mousePressed[0] = mousePressed[1] = false;
    //io.MouseWheel = 0;

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_MOUSEWHEEL:
            if (event.wheel.y < 0)
            {
                //ImGui::GetIO().MouseWheel = -1.0f;
            }
            else
            {
                //ImGui::GetIO().MouseWheel = 1.0f;
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
                if (keycode >= 32 && keycode <= 255)
                {
                    //printable ASCII characters
                    //ImGui::GetIO().AddInputCharacter((char)keycode);
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
                //ImGui_WindowResize();

                break;
            }
            break;

        case SDL_KEYDOWN:
        case SDL_KEYUP:
        {
            if (event.type == SDL_KEYDOWN)
            {
                if (event.key.keysym.sym == 13)
                {
                    const Uint32 windowFlags = SDL_GetWindowFlags(mWindow);
                    bool isFullScreen = ((windowFlags & SDL_WINDOW_FULLSCREEN_DESKTOP) || (windowFlags & SDL_WINDOW_FULLSCREEN));
                    SDL_SetWindowFullscreen(mWindow, isFullScreen ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
                    //ImGui_WindowResize();
                }

            }

            const SDL_Scancode key = SDL_GetScancodeFromKey(event.key.keysym.sym);
            if (key == SDL_SCANCODE_ESCAPE)
            {
                mFmv->Stop();
            }

            if (key >= 0 && key < 512)
            {
                SDL_Keymod modstate = SDL_GetModState();

                //ImGuiIO& io = ImGui::GetIO();
                //io.KeysDown[key] = event.type == SDL_KEYDOWN;
                //io.KeyCtrl = (modstate & KMOD_CTRL) != 0;
                //io.KeyShift = (modstate & KMOD_SHIFT) != 0;
            }
        }
            break;
        }
    }

    UpdateImGui();

    // TODO: Move into state machine
    //mFmv.Play("INGRDNT.DDV");
    mFmv->Update();
    mSound->Update();
    mLevel->Update();
}


void Engine::Render()
{
    int w, h;
    SDL_GetWindowSize(mWindow, &w, &h);
    mRenderer->beginFrame(w, h);

    DebugRender();

    mFmv->Render(mRenderer.get(), w, h);
    mSound->Render(w, h);
    mLevel->Render(mRenderer.get(), w, h);

    uint8_t testPixels[4*3] = {
        255, 0, 0,
        0, 255, 0,
        0, 255, 0,
        255, 0, 0,
    };
    int tex = mRenderer->createTexture(testPixels, 2, 2, PixelFormat_RGB24);
    mRenderer->drawQuad(tex, 10, 10, 200, 200);
    mRenderer->destroyTexture(tex);

    mRenderer->fillColor(255, 0, 0, 255);
    mRenderer->drawText(10, 10, "Woot");

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
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    //SDL_GL_SetAttribute( SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG ); // May be a performance booster in *nix?
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
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


void Engine::InitImGui()
{
    //ImGuiIO& io = ImGui::GetIO();
    //ImGui_WindowResize();
    //io.RenderDrawListsFn = ImImpl_RenderDrawLists;

    // Build texture
    unsigned char* pixels;
    int width, height;
    //io.Fonts->GetTexDataAsAlpha8(&pixels, &width, &height);

    // Create texture
    //glGenTextures(1, &g_FontTexture);
    //glBindTexture(GL_TEXTURE_2D, g_FontTexture);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, width, height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, pixels);

    // Store our identifier
    //io.Fonts->TexID = (void *)(intptr_t)g_FontTexture;

    // Cleanup (don't clear the input data if you want to append new fonts later)
    //io.Fonts->ClearInputData();
    //io.Fonts->ClearTexData();

    //
    // setup SDL2 keymapping
    //
    //io.KeyMap[ImGuiKey_Tab] = SDL_GetScancodeFromKey(SDLK_TAB);
    //io.KeyMap[ImGuiKey_LeftArrow] = SDL_GetScancodeFromKey(SDLK_LEFT);
    //io.KeyMap[ImGuiKey_RightArrow] = SDL_GetScancodeFromKey(SDLK_RIGHT);
    //io.KeyMap[ImGuiKey_UpArrow] = SDL_GetScancodeFromKey(SDLK_UP);
    //io.KeyMap[ImGuiKey_DownArrow] = SDL_GetScancodeFromKey(SDLK_DOWN);
    //io.KeyMap[ImGuiKey_Home] = SDL_GetScancodeFromKey(SDLK_HOME);
    //io.KeyMap[ImGuiKey_End] = SDL_GetScancodeFromKey(SDLK_END);
    //io.KeyMap[ImGuiKey_Delete] = SDL_GetScancodeFromKey(SDLK_DELETE);
    //io.KeyMap[ImGuiKey_Backspace] = SDL_GetScancodeFromKey(SDLK_BACKSPACE);
    //io.KeyMap[ImGuiKey_Enter] = SDL_GetScancodeFromKey(SDLK_RETURN);
    //io.KeyMap[ImGuiKey_Escape] = SDL_GetScancodeFromKey(SDLK_ESCAPE);
    //io.KeyMap[ImGuiKey_A] = SDLK_a;
    //io.KeyMap[ImGuiKey_C] = SDLK_c;
    //io.KeyMap[ImGuiKey_V] = SDLK_v;
    //io.KeyMap[ImGuiKey_X] = SDLK_x;
    //io.KeyMap[ImGuiKey_Y] = SDLK_y;
    //io.KeyMap[ImGuiKey_Z] = SDLK_z;

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
