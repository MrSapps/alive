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

Engine::Engine()
{

}

Engine::~Engine()
{
    destroy_gui(gui);

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

// TODO: Move gui drawing to own file
void drawButton(void *void_rend, float x, float y, float w, float h, bool down, bool hover)
{
    Renderer *rend = (Renderer*)void_rend;
    float cornerRadius = 4.0f;
    Color gradBegin = { 1.f, 1.f, 1.f, 64/255.f };
    Color gradEnd = { 0.f, 0.f, 0.f, 64/255.f };

    Color bgColor = { 0.3f, 0.3f, 0.3f, 0.5f };

    if (down)
    {
        Color begin = { 0.f, 0.f, 0.f, 64/255.f };
        Color end = { 0.3f, 0.3f, 0.3f, 64/255.f };
        gradBegin = begin;
        gradEnd = end;
    }

    RenderPaint overlay = rend->linearGradient(x, y, x, y + h,
                                              gradBegin,
                                              gradEnd);

    rend->beginPath();
    rend->roundedRect(x + 1, y + 1, w - 2, h - 2, cornerRadius - 1);

    rend->fillColor(bgColor);
    rend->fill();

    rend->fillPaint(overlay);
    rend->fill();

    Color outlineColor = Color{ 0, 0, 0, 0.4f };
    if (hover && !down)
    {
        outlineColor.r = 1.f;
        outlineColor.g = 1.f;
        outlineColor.b = 1.f;
    }
    rend->beginPath();
    rend->roundedRect(x + 0.5f, y + 0.5f, w - 1, h - 1, cornerRadius - 0.5f);
    rend->strokeColor(outlineColor);
    rend->stroke();
}

void drawText(void *void_rend, float x, float y, const char *text, float font_size)
{
    Renderer *rend = (Renderer*)void_rend;

    rend->fontSize(font_size);
    rend->textAlign(TEXT_ALIGN_LEFT | TEXT_ALIGN_TOP);
    rend->fillColor(Color{ 1.f, 1.f, 1.f, 160/255.f });
    rend->text(x, y, text);
}

void calcTextSize(float ret[2], void *void_rend, const char *text, float font_size)
{
    Renderer *rend = (Renderer*)void_rend;

    rend->fontSize(font_size);
    rend->textAlign(TEXT_ALIGN_LEFT | TEXT_ALIGN_TOP);
    float bounds[4];
    rend->textBounds(0, 0, text, bounds);
    ret[0] = bounds[2] - bounds[0];
    ret[1] = bounds[3] - bounds[1];
}

void Engine::InitSubSystems()
{
    mRenderer = std::make_unique<Renderer>((mFileSystem.BasePath() + "/data/Roboto-Regular.ttf").c_str());
    mFmv = std::make_unique<Fmv>(mGameData, mAudioHandler, mFileSystem);
    mSound = std::make_unique<Sound>(mGameData, mAudioHandler, mFileSystem);
    mLevel = std::make_unique<Level>(mGameData, mAudioHandler, mFileSystem);

    { // Init gui system
        GuiCallbacks callbacks = { 0 };
        callbacks.user_data = mRenderer.get();
        callbacks.draw_button = drawButton;
        callbacks.draw_text = drawText;
        callbacks.calc_text_size = calcTextSize;
        gui = create_gui(callbacks);
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
            gui->key_state[i] = 0;
        gui->cursor_pos.x = -1;
        gui->cursor_pos.y = -1;
    }

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
        case SDL_MOUSEBUTTONUP:
        case SDL_MOUSEBUTTONDOWN:
        {
            int guiKey = -1;
            if (event.button.button == SDL_BUTTON(SDL_BUTTON_LEFT))
                guiKey = GUI_KEY_LMB;

            if (guiKey >= 0)
            {
                  int state = gui->key_state[guiKey];
                  if (event.type == SDL_MOUSEBUTTONUP)
                  {
                      state |= GUI_KEYSTATE_RELEASED_BIT;
                  }
                  else
                  {
                      state |= GUI_KEYSTATE_PRESSED_BIT;
                  }
                  gui->key_state[GUI_KEY_LMB] = state;
            }

        } break;
        case SDL_KEYDOWN:
        case SDL_KEYUP:
        {
            if (event.key.keysym.sym == 13)
            {
                const Uint32 windowFlags = SDL_GetWindowFlags(mWindow);
                bool isFullScreen = ((windowFlags & SDL_WINDOW_FULLSCREEN_DESKTOP) || (windowFlags & SDL_WINDOW_FULLSCREEN));
                SDL_SetWindowFullscreen(mWindow, isFullScreen ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
                //ImGui_WindowResize();
            }

            const SDL_Scancode key = SDL_GetScancodeFromKey(event.key.keysym.sym);
            if (key == SDL_SCANCODE_ESCAPE)
            {
                mFmv->Stop();
            }

            SDL_Keymod modstate = SDL_GetModState();

            break;
        }
        }
    }

    { // Set rest of gui input state which wasn't set in event polling loop
        SDL_PumpEvents();

        int mouse_x, mouse_y;
        SDL_GetMouseState(&mouse_x, &mouse_y);
        gui->cursor_pos.x = mouse_x;
        gui->cursor_pos.y = mouse_y;

        if (SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_LEFT))
            gui->key_state[GUI_KEY_LMB] |= GUI_KEYSTATE_DOWN_BIT;
    }

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
    mRenderer->drawQuad(tex, 50, 50, 200, 200);

#if 0
    mRenderer->strokeColor(Color{ 0, 0, 0, 1 });
    mRenderer->strokeWidth(5);

    mRenderer->beginPath();
    mRenderer->moveTo(10, 10);
    mRenderer->lineTo(100, 150);
    mRenderer->lineTo(100, 50);
    mRenderer->closePath();
    mRenderer->stroke();


#endif

    gui_begin(gui, "background");
    if (gui_button(gui, "Test button"))
        LOG("BUTTON PRESSED");
    gui_next_row(gui);
    gui_hor_space(gui);
    gui_button(gui, "This is also a button");
    gui_next_row(gui);
    gui_hor_space(gui);
    gui_button(gui, "12394857349857");
    gui_end(gui);

    mRenderer->drawQuad(tex, 20, 20, 30, 30);

    mRenderer->destroyTexture(tex);

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
