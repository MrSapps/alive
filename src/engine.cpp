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
#include "gui.hpp"

#include "oddlib/anim.hpp"

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

//static bool mousePressed[4] = { false, false };

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
void drawButton(void *void_rend, float x, float y, float w, float h, bool down, bool hover, int layer, GuiScissor *s)
{
    Renderer *rend = (Renderer*)void_rend;
    rend->beginLayer(layer);
    if (s)
        rend->scissor(1.f*s->pos.x, 1.f*s->pos.y, 1.f*s->size.x, 1.f*s->size.y);
    else
        rend->resetScissor();

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

    Color outlineColor = Color{ 0, 0, 0, 0.3f };
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

    rend->endLayer();
}

void drawCheckBox(void *void_rend, float x, float y, float w, bool checked, bool /*down*/, bool hover, int layer, GuiScissor *s)
{
    Renderer *rend = (Renderer*)void_rend;
    rend->beginLayer(layer);
    if (s)
        rend->scissor(1.f*s->pos.x, 1.f*s->pos.y, 1.f*s->size.x, 1.f*s->size.y);
    else
        rend->resetScissor();

    RenderPaint bg;

    rend->beginPath();

    if (checked)
        bg = rend->boxGradient(x + 2, y + 2, x + w - 2, x + w - 2, 3, 3, Color{ 0.5f, 1.f, 0.5f, 92 / 255.f }, Color{ 0.5f, 1.f, 0.5f, 30/255.f });
    else
        bg = rend->boxGradient(x + 2, y + 2, x + w - 2, x + w - 2, 3, 3, Color{ 0.f, 0.f, 0.f, 30 / 255.f }, Color{ 0.f, 0.f, 0.f, 92/255.f });
    rend->roundedRect(x + 1, y + 1, w - 2, w - 2, 3);
    rend->fillPaint(bg);
    rend->fill();

    rend->strokeWidth(1.0f);
    if (hover)
        rend->strokeColor(Color{ 1.f, 1.f, 1.f, 0.3f });
    else
        rend->strokeColor(Color{ 0.f, 0.f, 0.f, 0.3f });
    rend->stroke();

    rend->endLayer();
}

void drawRadioButton(void *void_rend, float x, float y, float w, bool checked, bool /*down*/, bool hover, int layer, GuiScissor *s)
{
    Renderer *rend = (Renderer*)void_rend;
    rend->beginLayer(layer);
    if (s)
        rend->scissor(1.f*s->pos.x, 1.f*s->pos.y, 1.f*s->size.x, 1.f*s->size.y);
    else
        rend->resetScissor();

    RenderPaint bg;

    rend->beginPath();

    if (checked)
        bg = rend->radialGradient(x + w/2 + 1, y + w/2 + 1, w/2 - 2, w/2, Color{ 0.5f, 1.f, 0.5f, 92 / 255.f }, Color{ 0.5f, 1.f, 0.5f, 30/255.f });
    else
        bg = rend->radialGradient(x + w/2 + 1, y + w/2 + 1, w/2 - 2, w/2, Color{ 0.f, 0.f, 0.f, 30 / 255.f }, Color{ 0.f, 0.f, 0.f, 92/255.f });
    rend->circle(x + w/2, y + w/2, w/2);
    rend->fillPaint(bg);
    rend->fill();

    rend->strokeWidth(1.0f);
    if (hover)
        rend->strokeColor(Color{ 1.f, 1.f, 1.f, 0.3f });
    else
        rend->strokeColor(Color{ 0.f, 0.f, 0.f, 0.3f });
    rend->stroke();

    rend->endLayer();
}

void drawTextBox(void *void_rend, float x, float y, float w, float h, bool active, bool hover, int layer, GuiScissor *s)
{
    Renderer *rend = (Renderer*)void_rend;
    rend->beginLayer(layer);
    if (s)
        rend->scissor(1.f*s->pos.x, 1.f*s->pos.y, 1.f*s->size.x, 1.f*s->size.y);
    else
        rend->resetScissor();

    RenderPaint bg = rend->boxGradient(x+1,y+1+1.5f, w-2,h-2, 3,4, Color{1.f,1.f,1.f,32/255.f}, Color{32/255.f,32/255.f,32/255.f,32/255.f});
    rend->beginPath();
    rend->roundedRect(x+1,y+1, w-2,h-2, 4-1);
    rend->fillPaint(bg);
    rend->fill();

    rend->beginPath();
    rend->roundedRect(x+0.5f,y+0.5f, w-1,h-1, 4-0.5f);
    if (hover || active)
        rend->strokeColor(Color{ 1.f, 1.f, 1.f, 0.3f });
    else
        rend->strokeColor(Color{ 0.f, 0.f, 0.f, 48/255.f });
    rend->stroke();

    rend->endLayer();
}

static const float g_gui_font_size = 16.f;

void drawText(void *void_rend, float x, float y, const char *text, int layer, GuiScissor *s)
{
    Renderer *rend = (Renderer*)void_rend;
    rend->beginLayer(layer);
    if (s)
        rend->scissor(1.f*s->pos.x, 1.f*s->pos.y, 1.f*s->size.x, 1.f*s->size.y);
    else
        rend->resetScissor();

    rend->fontSize(g_gui_font_size);
    rend->textAlign(TEXT_ALIGN_LEFT | TEXT_ALIGN_TOP);
    rend->fontBlur(0);
    rend->fillColor(Color{ 1.f, 1.f, 1.f, 160/255.f });
    rend->text(x, y, text);

    rend->endLayer();
}

void calcTextSize(float ret[2], void *void_rend, const char *text, int layer)
{
    Renderer *rend = (Renderer*)void_rend;
    rend->beginLayer(layer);

    rend->fontSize(g_gui_font_size);
    rend->textAlign(TEXT_ALIGN_LEFT | TEXT_ALIGN_TOP);
    rend->fontBlur(0);
    float bounds[4];
    rend->textBounds(0, 0, text, bounds);
    ret[0] = bounds[2] - bounds[0];
    ret[1] = bounds[3] - bounds[1];

    rend->endLayer();
}

void drawWindow(void *void_rend, float x, float y, float w, float h, float titleBarHeight, const char *title, bool focus, int layer)
{
    Renderer *rend = (Renderer*)void_rend;
    rend->beginLayer(layer); // Makes window reordering possible
    rend->resetScissor();

    float cornerRadius = 3.0f;
    RenderPaint shadowPaint;
    RenderPaint headerPaint;

    // Window
    rend->beginPath();
    rend->roundedRect(x, y, w, h, cornerRadius);
    rend->fillColor(Color{ 28/255.f, 30/255.f, 34/255.f, 220/255.f });
    //	nvgFillColor(vg, nvgRGBA(0,0,0,128));
    rend->fill();

    // Drop shadow
    shadowPaint = rend->boxGradient(x, y + 2, w, h, cornerRadius * 2, 10, Color{ 0.f, 0.f, 0.f, 128 / 255.f }, Color{ 0.f, 0.f, 0.f, 0.f });
    rend->beginPath();
    rend->rect(x - 10, y - 10, w + 20, h + 30);
    rend->roundedRect(x, y, w, h, cornerRadius);
    rend->solidPathWinding(false);
    rend->fillPaint(shadowPaint);
    rend->fill();
    rend->solidPathWinding(true);

    // Header
    if (focus)
        headerPaint = rend->linearGradient(x, y, x, y + 15, Color{ 1.0f, 1.f, 1.0f, 16 / 255.f }, Color{ 0.f, 0.0f, 0.f, 32/255.f });
    else
        headerPaint = rend->linearGradient(x, y, x, y + 15, Color{ 1.f, 1.f, 1.f, 16 / 255.f }, Color{ 1.f, 1.f, 1.f, 16/255.f });
    rend->beginPath();
    rend->roundedRect(x + 1, y, w - 2, titleBarHeight, cornerRadius - 1);
    rend->fillPaint(headerPaint);
    rend->fill();
    rend->beginPath();
    rend->moveTo(x + 0.5f, y - 0.5f + titleBarHeight);
    rend->lineTo(x + 0.5f + w - 1, y - 0.5f + titleBarHeight);
    rend->strokeColor(Color{ 0, 0, 0, 64/255.f });
    rend->stroke();

    rend->fontSize(18.0f);
    rend->textAlign(TEXT_ALIGN_CENTER | TEXT_ALIGN_MIDDLE);
    rend->fontBlur(3);
    rend->fillColor(Color{ 0.f, 0.f, 0.f, 160/255.f });
    rend->text(x + w / 2, y + 16 + 1, title);

    rend->fontBlur(0);
    rend->fillColor(Color{ 230/255.f, 230/255.f, 230/255.f, 200/255.f });
    rend->text(x + w / 2, y + 16, title);

    rend->endLayer();
}

void Engine::InitSubSystems()
{
    mRenderer = std::make_unique<Renderer>((mFileSystem.GameData().BasePath() + "/data/Roboto-Regular.ttf").c_str());
    mFmv = std::make_unique<DebugFmv>(mGameData, mAudioHandler, mFileSystem);
    mSound = std::make_unique<Sound>(mGameData, mAudioHandler, mFileSystem);
    mLevel = std::make_unique<Level>(mGameData, mAudioHandler, mFileSystem);

    { // Init gui system
        GuiCallbacks callbacks = { 0 };
        callbacks.user_data = mRenderer.get();
        callbacks.draw_button = drawButton;
        callbacks.draw_checkbox = drawCheckBox;
        callbacks.draw_radiobutton = drawRadioButton;
        callbacks.draw_textbox = drawTextBox;
        callbacks.draw_text = drawText;
        callbacks.calc_text_size = calcTextSize;
        callbacks.draw_window = drawWindow;
        mGui = create_gui(callbacks);
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
        mGui->cursor_pos.x = -1;
        mGui->cursor_pos.y = -1;
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

            if (guiKey >= 0)
            {
                  uint8_t state = mGui->key_state[guiKey];
                  if (event.type == SDL_MOUSEBUTTONUP)
                  {
                      state |= GUI_KEYSTATE_RELEASED_BIT;
                  }
                  else
                  {
                      state |= GUI_KEYSTATE_PRESSED_BIT;
                  }
                  mGui->key_state[GUI_KEY_LMB] = state;
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
        mGui->cursor_pos.x = mouse_x;
        mGui->cursor_pos.y = mouse_y;

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

        float xpos = 300.0f - (frame.mOffX);
        float ypos = 300.0f;// -(frame.mOffY);
        // LOG_INFO("Pos " << xpos << "," << ypos);
        rend.drawQuad(textureId, xpos, ypos, static_cast<float>(frame.mFrame->w * 2), static_cast<float>(frame.mFrame->h * 2));

        rend.destroyTexture(textureId);
    }
};

void Engine::Render()
{
    int w, h;
    SDL_GetWindowSize(mWindow, &w, &h);
    mGui->host_win_size = v2i(w, h);

    mRenderer->beginFrame(w, h);
    gui_begin(mGui, "background");

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
        };


        static EditorUi editor;

        mGui->next_window_pos = v2i(50, 50);
        gui_begin_window(mGui, "Browsers", v2i(200, 130));
        gui_checkbox(mGui, "resPathsOpen|Resource paths", &editor.resPathsOpen);
        gui_checkbox(mGui, "fmvBrowserOpen|FMV browser", &editor.fmvBrowserOpen);
        gui_checkbox(mGui, "soundBrowserOpen|Sound browser", &editor.soundBrowserOpen);
        gui_checkbox(mGui, "levelBrowserOpen|Level browser", &editor.levelBrowserOpen);
        gui_checkbox(mGui, "animationBrowserOpen|Animation browser", &editor.animationBrowserOpen);

        gui_end_window(mGui);

        mGui->next_window_pos = v2i(300, 50);

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
                static auto stream = mFileSystem.ResourcePaths().Open("FD.LVL");
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

            mGui->next_window_pos = v2i(350, 50);
            gui_begin_window(mGui, "Animations", v2i(200, 130));
            for (auto& res : resources)
            {
                gui_checkbox(mGui, res.first.c_str(), &res.second->mDisplay);
            }
  
            gui_end_window(mGui);
            
            for (auto& res : resources)
            {
                if (res.second->mDisplay)
                {
                    Oddlib::LvlArchive::FileChunk* chunk = res.second->mFileChunk;
                    if (!res.second->mAnim)
                    {
                        res.second->mAnim = Oddlib::LoadAnimations(*chunk->Stream(), false);
                    }

                    res.second->Animate(*mRenderer);
                }
            }  
        }
    }

    gui_end(mGui);
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

