#include "engine.hpp"
#include <iostream>
#include "oddlib/masher.hpp"
#include "oddlib/exceptions.hpp"
#include "logger.hpp"
#include "jsonxx/jsonxx.h"
#include <fstream>
#include "proxy_sqrat.hpp"
#include "alive_version.h"
#include "core/audiobuffer.hpp"
#include "sound.hpp"
#include "gridmap.hpp"
#include "GameSelectionState.hpp"
#include "gamefilesystem.hpp"
#include "fmv.hpp"
#include "rendererfactory.hpp"
#include "rungamestate.hpp"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#undef DeleteFile

#include "rsc\resource.h"
#include "SDL_syswm.h"
#include "stdthread.h"
#include "imgui/imgui.h"

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

InputMapping::InputMapping()
{
    // Keyboard
    mKeyBoardConfig[Actions::eLeft] =         { 1u, { SDL_SCANCODE_LEFT } };
    mKeyBoardConfig[Actions::eRight] =        { 1u, { SDL_SCANCODE_RIGHT } };
    mKeyBoardConfig[Actions::eUp] =           { 1u, { SDL_SCANCODE_UP } };
    mKeyBoardConfig[Actions::eDown] =         { 1u, { SDL_SCANCODE_DOWN } };
    mKeyBoardConfig[Actions::eChant] =        { 1u, { SDL_SCANCODE_0 } };
    mKeyBoardConfig[Actions::eRun] =          { 1u, { SDL_SCANCODE_LSHIFT,    SDL_SCANCODE_RSHIFT } };
    mKeyBoardConfig[Actions::eSneak] =        { 1u, { SDL_SCANCODE_LALT,      SDL_SCANCODE_RALT } };
    mKeyBoardConfig[Actions::eJump] =         { 1u, { SDL_SCANCODE_SPACE } };
    mKeyBoardConfig[Actions::eThrow] =        { 1u, { SDL_SCANCODE_Z } };
    mKeyBoardConfig[Actions::eAction] =       { 1u, { SDL_SCANCODE_LCTRL,     SDL_SCANCODE_RCTRL } };
    mKeyBoardConfig[Actions::eRollOrFart] =   { 1u, { SDL_SCANCODE_X } };
    mKeyBoardConfig[Actions::eGameSpeak1] =   { 1u, { SDL_SCANCODE_1 } };
    mKeyBoardConfig[Actions::eGameSpeak2] =   { 1u, { SDL_SCANCODE_2 } };
    mKeyBoardConfig[Actions::eGameSpeak3] =   { 1u, { SDL_SCANCODE_3 } };
    mKeyBoardConfig[Actions::eGameSpeak4] =   { 1u, { SDL_SCANCODE_4 } };
    mKeyBoardConfig[Actions::eGameSpeak5] =   { 1u, { SDL_SCANCODE_5 } };
    mKeyBoardConfig[Actions::eGameSpeak6] =   { 1u, { SDL_SCANCODE_6 } };
    mKeyBoardConfig[Actions::eGameSpeak7] =   { 1u, { SDL_SCANCODE_7 } };
    mKeyBoardConfig[Actions::eGameSpeak8] =   { 1u, { SDL_SCANCODE_8 } };
    mKeyBoardConfig[Actions::eBack] =         { 1u, { SDL_SCANCODE_ESCAPE } };

    // Game pad
    mGamePadConfig[Actions::eLeft] =          { 1u, { SDL_CONTROLLER_BUTTON_DPAD_LEFT } };
    mGamePadConfig[Actions::eRight] =         { 1u, { SDL_CONTROLLER_BUTTON_DPAD_RIGHT } };
    mGamePadConfig[Actions::eUp] =            { 1u, { SDL_CONTROLLER_BUTTON_DPAD_UP } };
    mGamePadConfig[Actions::eDown] =          { 1u, { SDL_CONTROLLER_BUTTON_DPAD_DOWN } };

    // TODO: Should actually be the trigger axes!
    mGamePadConfig[Actions::eChant] =         { 2u, {
                                                SDL_CONTROLLER_BUTTON_LEFTSHOULDER,
                                                SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,
                                                SDL_CONTROLLER_BUTTON_LEFTSTICK,
                                                SDL_CONTROLLER_BUTTON_RIGHTSTICK } };

    mGamePadConfig[Actions::eRun] =           { 1u, { SDL_CONTROLLER_BUTTON_RIGHTSHOULDER } };

    // TODO: Should actually be the trigger axes!
    mGamePadConfig[Actions::eSneak] =         { 1u, { SDL_CONTROLLER_BUTTON_RIGHTSTICK } };

    mGamePadConfig[Actions::eJump] =          { 1u, { SDL_CONTROLLER_BUTTON_Y } };
    mGamePadConfig[Actions::eThrow] =         { 1u, { SDL_CONTROLLER_BUTTON_B } };
    mGamePadConfig[Actions::eAction] =        { 1u, { SDL_CONTROLLER_BUTTON_X } };
    mGamePadConfig[Actions::eRollOrFart] =    { 1u, { SDL_CONTROLLER_BUTTON_A } };
    mGamePadConfig[Actions::eGameSpeak1] =    { 2u, { SDL_CONTROLLER_BUTTON_LEFTSHOULDER,  SDL_CONTROLLER_BUTTON_Y } };
    mGamePadConfig[Actions::eGameSpeak2] =    { 2u, { SDL_CONTROLLER_BUTTON_LEFTSHOULDER,  SDL_CONTROLLER_BUTTON_B } };
    mGamePadConfig[Actions::eGameSpeak3] =    { 2u, { SDL_CONTROLLER_BUTTON_LEFTSHOULDER,  SDL_CONTROLLER_BUTTON_A } };
    mGamePadConfig[Actions::eGameSpeak4] =    { 2u, { SDL_CONTROLLER_BUTTON_LEFTSHOULDER,  SDL_CONTROLLER_BUTTON_X } };
    mGamePadConfig[Actions::eGameSpeak5] =    { 2u, { SDL_CONTROLLER_BUTTON_RIGHTSHOULDER ,SDL_CONTROLLER_BUTTON_Y } };
    mGamePadConfig[Actions::eGameSpeak6] =    { 2u, { SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, SDL_CONTROLLER_BUTTON_B } };
    mGamePadConfig[Actions::eGameSpeak7] =    { 2u, { SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, SDL_CONTROLLER_BUTTON_A } };
    mGamePadConfig[Actions::eGameSpeak8] =    { 2u, { SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, SDL_CONTROLLER_BUTTON_X } };
    mGamePadConfig[Actions::eBack] =          { 1u, { SDL_CONTROLLER_BUTTON_START } };
}

void InputMapping::Update(const InputState& input)
{
    for (const auto& cfg : mKeyBoardConfig)
    {
        u32 numDowns = 0;
        for (const auto& key : cfg.second.mKeys)
        {
            if (input.mKeys[key].RawDownState())
            {
                numDowns++;
            }
        }
        mActions.SetRawDownState(cfg.first, numDowns >= cfg.second.mNumberOfKeysRequired);
    }

    if (input.ActiveController())
    {
        for (const auto& cfg : mGamePadConfig)
        {
            // The keyboard key isn't pressed so take the controller key state
            if (mActions.RawDownState(cfg.first) == false)
            {
                u32 numDowns = 0;
                for (const auto& key : cfg.second.mButtons)
                {
                    if (input.ActiveController()->mGamePadButtons[key].RawDownState())
                    {
                        numDowns++;
                    }
                }
                mActions.SetRawDownState(cfg.first, numDowns >= cfg.second.mNumberOfKeysRequired);
            }
        }
    }

    mActions.UpdateStates();
}

/*static*/ void Actions::RegisterScriptBindings()
{
    Sqrat::Class<Actions> c(Sqrat::DefaultVM::Get(), "Actions");
    c
        .StaticFunc("Left", &Actions::Left)
        .StaticFunc("Right", &Actions::Right)
        .StaticFunc("Up", &Actions::Up)
        .StaticFunc("Down", &Actions::Down)
        .StaticFunc("Chant", &Actions::Chant)
        .StaticFunc("Run", &Actions::Run)
        .StaticFunc("Sneak", &Actions::Sneak)
        .StaticFunc("Jump", &Actions::Jump)
        .StaticFunc("Throw", &Actions::Throw)
        .StaticFunc("Action", &Actions::Action)
        .StaticFunc("RollOrFart", &Actions::RollOrFart)
        .StaticFunc("GameSpeak1", &Actions::GameSpeak1)
        .StaticFunc("GameSpeak2", &Actions::GameSpeak2)
        .StaticFunc("GameSpeak3", &Actions::GameSpeak3)
        .StaticFunc("GameSpeak4", &Actions::GameSpeak4)
        .StaticFunc("GameSpeak5", &Actions::GameSpeak5)
        .StaticFunc("GameSpeak6", &Actions::GameSpeak6)
        .StaticFunc("GameSpeak7", &Actions::GameSpeak7)
        .StaticFunc("GameSpeak8", &Actions::GameSpeak8)
        .StaticFunc("Back", &Actions::Back)
        .Var("IsPressed", &Actions::mIsPressed)
        .Var("IsReleased", &Actions::mIsReleased)
        .Var("IsHeld", &Actions::mIsDown)
        .Ctor();

    Sqrat::RootTable().Bind("Actions", c);
}

Engine::Engine(const std::vector<std::string>& commandLineArguments)
    : mAudioHandler(1024, 44100)
{
    for (const std::string& argument : commandLineArguments)
    {
        if (string_util::iequals("-opengl", argument))
        {
            mTryDirectX9 = false;
        }
        else if (string_util::iequals("-directx9", argument))
        {
#ifndef _WIN32
            LOG_WARNING("DirectX9 is not supported on this platform");
#else
            mTryDirectX9 = true;
#endif
        }
    }
}

Engine::~Engine()
{
    TRACE_ENTRYEXIT;

    ImGui::Shutdown();

    mSound.reset();
    mRunGameState.reset();
    if (mRenderer)
    {
        mRenderer->ShutDown();
    }
    mRenderer.reset();

    SDL_DestroyWindow(mWindow);
    SDL_Quit();
}

bool Engine::Init()
{

    // load the list of data paths (if any) and discover what they are
    mFileSystem = std::make_unique<GameFileSystem>();
    if (!mFileSystem->Init())
    {
        LOG_ERROR("File system init failure");
        return false;
    }

    //InitResources();

    if (!InitSDL())
    {
        LOG_ERROR("SDL init failure");
        return false;
    }

    InitSubSystems();

    Debugging().mInput = &mInputState;

    mState = EngineStates::eEngineInit;

    return true;
}

void ScriptLogTrace(const char* msg) { if (msg) { LOG_NOFUNC_TRACE(msg); } else  { LOG_NOFUNC_TRACE("nil"); } }
void ScriptLogInfo(const char* msg) { if (msg) { LOG_NOFUNC_INFO(msg); } else { LOG_NOFUNC_INFO("nil"); } }
void ScriptLogWarning(const char* msg) { if (msg) { LOG_NOFUNC_WARNING(msg); } else { LOG_NOFUNC_WARNING("nil"); } }
void ScriptLogError(const char* msg) { if (msg) { LOG_NOFUNC_ERROR(msg); } else { LOG_NOFUNC_ERROR("nil"); } }

void Engine::Include(const std::string& scriptName)
{
    SquirrelVm::CompileAndRun(*mResourceLocator, scriptName);
}

void Engine::BindScriptTypes()
{
    Sqrat::RootTable().Func("log_info", ScriptLogInfo);
    Sqrat::RootTable().Func("log_trace", ScriptLogTrace);
    Sqrat::RootTable().Func("log_warning", ScriptLogWarning);
    Sqrat::RootTable().Func("log_error", ScriptLogError);

    Sqrat::Class<Engine, Sqrat::NoConstructor<Engine>> engine(Sqrat::DefaultVM::Get(), "Engine");
    engine.Func("include", &Engine::Include);
    Sqrat::RootTable().Bind("Engine", engine);
    // TODO: Use InstanceBinder
    Sqrat::RootTable().SetInstance("gEngine", this);

    Oddlib::IStream::RegisterScriptBindings();
    Actions::RegisterScriptBindings();
    MapObject::RegisterScriptBindings();
    ObjRect::RegisterScriptBindings();
    GridMap::RegisterScriptBindings();
    Sound::RegisterScriptBindings();
    Fmv::RegisterScriptBindings();
}

void Engine::InitSubSystems()
{
    TRACE_ENTRYEXIT;

    BindScriptTypes();

    mRenderer = RendererFactory::Create(mWindow, mTryDirectX9);

    mRenderer->Init
    (
        (mFileSystem->FsPath() + "data/fonts/Roboto-Regular.ttf").c_str(),
        (mFileSystem->FsPath() + "data/fonts/Roboto-Italic.ttf").c_str(),
        (mFileSystem->FsPath() + "data/fonts/Roboto-Bold.ttf").c_str()
    );

    InitImGui();

    mInputState.AddControllers();
}

void Engine::RunInitScript()
{
    SquirrelVm::CompileAndRun(*mResourceLocator, "main.nut");

    LOG_INFO("Calling script init()");
    Sqrat::Function initFunc(Sqrat::RootTable(), "init");
    initFunc.Execute();
    SquirrelVm::CheckError();
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

static char* WindowTitle(const char* rendererName, f32 fps)
{
    static char buffer[128] = {};
    sprintf(buffer, ALIVE_VERSION_NAME_STR, rendererName, fps);
    return buffer;
}

int Engine::Run()
{
    BasicFramesPerSecondCounter fpsCounter;
    auto startTime = THighResClock::now();

    while (mState != EngineStates::eQuit)
    {
        // Limit update to 60fps
        const THighResClock::duration totalRunTime = THighResClock::now() - startTime;
        const Sint64 timePassed = std::chrono::duration_cast<std::chrono::nanoseconds>(totalRunTime).count();


        if (timePassed >= 16666666)
        {
            Update();
            ImGui::Render();
            startTime = THighResClock::now();
        }

        Render();
        fpsCounter.Update([&](f32 fps)
        {
            SDL_SetWindowTitle(mWindow, WindowTitle(mRenderer->Name(), fps));
        });
    }

    mRenderer->DestroyTexture(mGuiFontHandle);

    return 0;
}

static bool mousePressed[4] = { false, false };
static ImVec2 mousePosScale(1.0f, 1.0f);

static void UpdateImGui()
{
    ImGuiIO& io = ImGui::GetIO();

    SDL_PumpEvents();

    // Setup inputs
    int mouse_x, mouse_y;
    SDL_GetMouseState(&mouse_x, &mouse_y);

    io.MousePos = ImVec2((float)mouse_x * mousePosScale.x, (float)mouse_y * mousePosScale.y);      // Mouse position, in pixels (set to -1,-1 if no mouse / on another screen, etc.)

    io.MouseDown[0] = mousePressed[0] || (SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_LEFT));
    io.MouseDown[1] = mousePressed[1] || (SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_RIGHT));
}

void Engine::InitImGui()
{
    ImGui::GetStyle().AntiAliasedLines = true;
    ImGui::GetStyle().AntiAliasedShapes = true;

    ImGuiIO& io = ImGui::GetIO();
    ImFontConfig config;
    config.OversampleH = 3;
    config.OversampleV = 3;
    std::string fontPath = mFileSystem->FsPath() + "data/fonts/Roboto-Regular.ttf";
    if (!io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 18.0f, &config))
    {
        throw Oddlib::Exception(("Failed to load font " + fontPath).c_str());
    }

    ImGui_WindowResize();

    // Build texture
    unsigned char* pixels = nullptr;
    int width = 0;
    int height = 0;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    // Create texture
    mGuiFontHandle = mRenderer->CreateTexture(AbstractRenderer::eTextureFormats::eRGBA, width, height, AbstractRenderer::eTextureFormats::eRGBA, pixels, true);

    // Store our identifier
    io.Fonts->TexID = mGuiFontHandle.mData;

    // Cleanup (don't clear the input data if you want to append new fonts later)
    io.Fonts->ClearInputData();
    io.Fonts->ClearTexData();

    // setup SDL2 keymapping
    io.KeyMap[ImGuiKey_Tab] = SDL_GetScancodeFromKey(SDLK_TAB);
    io.KeyMap[ImGuiKey_LeftArrow] = SDL_GetScancodeFromKey(SDLK_LEFT);
    io.KeyMap[ImGuiKey_RightArrow] = SDL_GetScancodeFromKey(SDLK_RIGHT);
    io.KeyMap[ImGuiKey_UpArrow] = SDL_GetScancodeFromKey(SDLK_UP);
    io.KeyMap[ImGuiKey_DownArrow] = SDL_GetScancodeFromKey(SDLK_DOWN);
    io.KeyMap[ImGuiKey_Home] = SDL_GetScancodeFromKey(SDLK_HOME);
    io.KeyMap[ImGuiKey_End] = SDL_GetScancodeFromKey(SDLK_END);
    io.KeyMap[ImGuiKey_Delete] = SDL_GetScancodeFromKey(SDLK_DELETE);
    io.KeyMap[ImGuiKey_Backspace] = SDL_GetScancodeFromKey(SDLK_BACKSPACE);
    io.KeyMap[ImGuiKey_Enter] = SDL_GetScancodeFromKey(SDLK_RETURN);
    io.KeyMap[ImGuiKey_Escape] = SDL_GetScancodeFromKey(SDLK_ESCAPE);
    io.KeyMap[ImGuiKey_A] = SDLK_a;
    io.KeyMap[ImGuiKey_C] = SDLK_c;
    io.KeyMap[ImGuiKey_V] = SDLK_v;
    io.KeyMap[ImGuiKey_X] = SDLK_x;
    io.KeyMap[ImGuiKey_Y] = SDLK_y;
    io.KeyMap[ImGuiKey_Z] = SDLK_z;


    ImGui::GetStyle().WindowTitleAlign = ImVec2(0.5f, 0.5f);
    ImGui::GetStyle().WindowRounding = 3.0f;

    auto& style = ImGui::GetStyle();

    style.Colors[ImGuiCol_Text] = ImVec4(1, 1, 1, 1);
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.86f, 0.93f, 0.89f, 0.28f);
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.13f, 0.14f, 0.17f, 1.00f);
    style.Colors[ImGuiCol_ChildWindowBg] = ImVec4(0.20f, 0.22f, 0.27f, 0.58f);
    style.Colors[ImGuiCol_Border] = ImVec4(0.31f, 0.31f, 1.00f, 0.00f);
    style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.92f, 0.18f, 0.29f, 0.78f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.20f, 0.22f, 0.27f, 0.75f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.20f, 0.22f, 0.27f, 0.47f);
    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.47f, 0.77f, 0.83f, 0.21f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.92f, 0.18f, 0.29f, 0.78f);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
    style.Colors[ImGuiCol_ComboBg] = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.71f, 0.22f, 0.27f, 1.00f);
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.47f, 0.77f, 0.83f, 0.14f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.47f, 0.77f, 0.83f, 0.14f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.92f, 0.18f, 0.29f, 0.86f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.92f, 0.18f, 0.29f, 0.76f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.92f, 0.18f, 0.29f, 0.86f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
    style.Colors[ImGuiCol_Column] = ImVec4(0.47f, 0.77f, 0.83f, 0.32f);
    style.Colors[ImGuiCol_ColumnHovered] = ImVec4(0.92f, 0.18f, 0.29f, 0.78f);
    style.Colors[ImGuiCol_ColumnActive] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
    style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.47f, 0.77f, 0.83f, 0.04f);
    style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.92f, 0.18f, 0.29f, 0.78f);
    style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
    style.Colors[ImGuiCol_CloseButton] = ImVec4(0.86f, 0.93f, 0.89f, 0.16f);
    style.Colors[ImGuiCol_CloseButtonHovered] = ImVec4(0.86f, 0.93f, 0.89f, 0.39f);
    style.Colors[ImGuiCol_CloseButtonActive] = ImVec4(0.86f, 0.93f, 0.89f, 1.00f);
    style.Colors[ImGuiCol_PlotLines] = ImVec4(0.86f, 0.93f, 0.89f, 0.63f);
    style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.86f, 0.93f, 0.89f, 0.63f);
    style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
    style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.92f, 0.18f, 0.29f, 0.43f);
    //style.Colors[ImGuiCol_TooltipBg] = ImVec4(0.47f, 0.77f, 0.83f, 0.72f);
    style.Colors[ImGuiCol_ModalWindowDarkening] = ImVec4(0.20f, 0.22f, 0.27f, 0.73f);
}

//TODO: ImGui shutdown/clean up
void Engine::ImGui_WindowResize()
{
    int w, h;
    int fb_w, fb_h;
    SDL_GetWindowSize(mWindow, &w, &h);
    SDL_GetWindowSize(mWindow, &fb_w, &fb_h); // Needs to be corrected for SDL Framebuffer
    mousePosScale.x = (float)fb_w / w;
    mousePosScale.y = (float)fb_h / h;

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)fb_w, (float)fb_h);  // Display size, in pixels. For clamping windows positions.
    //    io.PixelCenterOffset = 0.0f;                        // Align OpenGL texels
}

void Engine::Update()
{
    ImGuiIO& io = ImGui::GetIO();
    io.MouseWheel = 0;

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_MOUSEWHEEL:
            if (event.wheel.y < 0)
            {
                ImGui::GetIO().MouseWheel = -1.0f;
            }
            else
            {
                ImGui::GetIO().MouseWheel = 1.0f;
            }
            break;

        case SDL_QUIT:
            mState = EngineStates::eQuit;
            break;

        case SDL_TEXTINPUT:
        {
            size_t len = strlen(event.text.text);
            for (size_t i = 0; i < len; i++)
            {
                uint32_t keycode = event.text.text[i];
                if (keycode >= 10 && keycode <= 255)
                {
                    ImGui::GetIO().AddInputCharacter((char)keycode);
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
                mRenderer->mSmoothCameraPosition = false;
                ImGui_WindowResize();
                break;
            }
            break;
        case SDL_MOUSEBUTTONUP:
        case SDL_MOUSEBUTTONDOWN:
        {
            mInputState.OnMouseButtonEvent(event.button);
        } break;

        case SDL_CONTROLLERDEVICEADDED:
            mInputState.AddController(event.cdevice);
            break;

        case SDL_CONTROLLERDEVICEREMOVED:
            mInputState.RemoveController(event.cdevice);
            break;

        case SDL_CONTROLLERBUTTONDOWN:
        case SDL_CONTROLLERBUTTONUP:
            mInputState.OnControllerButton(event.cbutton);
            break;

        case SDL_CONTROLLERAXISMOTION:
            mInputState.OnControllerAxis(event.caxis);
            break;

        case SDL_KEYDOWN:
        case SDL_KEYUP:
        {
            mInputState.OnKeyEvent(event.key);

            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == 13)
            {
                const u32 windowFlags = SDL_GetWindowFlags(mWindow);
                bool isFullScreen = ((windowFlags & SDL_WINDOW_FULLSCREEN_DESKTOP) || (windowFlags & SDL_WINDOW_FULLSCREEN));
                SDL_SetWindowFullscreen(mWindow, isFullScreen ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
                //OnWindowResize();
            }

            const SDL_Scancode key = SDL_GetScancodeFromKey(event.key.keysym.sym);
            if (key >= 0 && key < 512)
            {
                SDL_Keymod modstate = SDL_GetModState();

                io.KeysDown[key] = event.type == SDL_KEYDOWN;
                io.KeyCtrl = (modstate & KMOD_CTRL) != 0;
                io.KeyShift = (modstate & KMOD_SHIFT) != 0;
            }

            break;
        }
        }
    }

    // Set rest of gui input state which wasn't set in event polling loop
    SDL_PumpEvents();

    int mouse_x, mouse_y;
    SDL_GetMouseState(&mouse_x, &mouse_y);

    mInputState.mMousePosition.mX = mouse_x;
    mInputState.mMousePosition.mY = mouse_y;

    ImGui::NewFrame();
    mInputState.Update();

    if (mState == EngineStates::eRunGameState)
    {
        Debugging().Update(mInputState);
    }

    UpdateImGui();

    switch (mState)
    {
    case EngineStates::eRunGameState:
        mState = mRunGameState->Update(mInputState, *mRenderer);
        break;
    case EngineStates::eGameSelection:
        mState = mGameSelectionScreen->Update(mInputState, *mRenderer);
        if (mState == EngineStates::eRunGameState)
        {
            mRunGameState->OnStart(mGameSelectionScreen->SelectedGame().GameScriptName(), mSound.get());
        }
        break;
    case EngineStates::ePlayFmv:
        mState = mPlayFmvState->Update(mInputState);
        break;
    case EngineStates::eEngineInit:
        {
            if (!mFuture)
            {
                mFuture = std::make_unique<std::future<void>>(std::async(std::launch::async, [&]()
                {
                    // At this point nothing else is reading the things this thread is writing, so
                    // should be thread safe.
                    InitResources();
                }));
            }

            if (mFuture->wait_for(std::chrono::seconds(0)) == std::future_status::ready)
            {
                mFuture = nullptr;
                mState = EngineStates::eGameSelection;

                mSound = std::make_unique<Sound>(mAudioHandler, *mResourceLocator, *mFileSystem);

                mRunGameState = std::make_unique<RunGameState>(*mResourceLocator, *mRenderer);
                mGameSelectionScreen = std::make_unique<GameSelectionState>(mGameDefinitions, *mResourceLocator, *mFileSystem);
                mPlayFmvState = std::make_unique<PlayFmvState>(mAudioHandler, *mResourceLocator);

                RunInitScript();
                mResourcesAreLoading = false;
            }
        }
        break;
    case EngineStates::eQuit:
        break;
    }
}

void Engine::Render()
{
    int w = 0;
    int h = 0;
    SDL_GetWindowSize(mWindow, &w, &h);

    mRenderer->UpdateCamera();
    mRenderer->BeginFrame(w, h);

    switch (mState)
    {
    case EngineStates::eRunGameState:
        mRunGameState->Render(*mRenderer);
        break;

    case EngineStates::eGameSelection:
        mGameSelectionScreen->Render(*mRenderer);
        break;

    case EngineStates::ePlayFmv:
        mPlayFmvState->Render(*mRenderer);
        break;

    case EngineStates::eEngineInit:
    case EngineStates::eQuit:
        mRenderer->Clear(0.0f, 0.0f, 0.0f);
        break;
    }

    if (mState == EngineStates::eRunGameState)
    {
        Debugging().Render(*mRenderer);
    }

    if (!mLoadingIcon)
    {
        std::unique_ptr<Oddlib::IStream> pngStream = mFileSystem->Open("{GameDir}/data/images/loading.png");
        if (pngStream)
        {
            mLoadingIcon = SDLHelpers::LoadPng(*pngStream, true);
        }

        if (!mLoadingIcon)
        {
            throw Oddlib::Exception("Failed to load {GameDir}/data/images/loading.png");
        }
    }

    if (mResourcesAreLoading)
    {
        TextureHandle hTexture = mRenderer->CreateTexture(AbstractRenderer::eTextureFormats::eRGBA, mLoadingIcon->w, mLoadingIcon->h, AbstractRenderer::eTextureFormats::eRGBA, mLoadingIcon->pixels, true);

        const f32 imagew = static_cast<f32>(mLoadingIcon->w);
        const f32 imageh = static_cast<f32>(mLoadingIcon->h);

        const f32 padding = 30.0f;

        const f32 xpos = static_cast<f32>(mRenderer->Width()) - imagew - padding;
        const f32 ypos = static_cast<f32>(mRenderer->Height()) - imageh - padding;

        mRenderer->TexturedQuad(hTexture,
            xpos,
            ypos,
            imagew,
            imageh,
            AbstractRenderer::eLayers::eFmv + 5, { 255, 255, 255, 255 }, AbstractRenderer::eNormal, AbstractRenderer::eScreen);

        mRenderer->DestroyTexture(hTexture);
    }

    mRenderer->EndFrame();
}

bool Engine::InitSDL()
{
    if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_EVENTS | SDL_INIT_HAPTIC) != 0)
    {
        LOG_ERROR("SDL_Init failed " << SDL_GetError());
        return false;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    std::string title = WindowTitle("", 0.0f);
    title = title.substr(0, title.find(")")+1);

    mWindow = SDL_CreateWindow(title.c_str(),
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!mWindow)
    {
        LOG_ERROR("Failed to create window: " << SDL_GetError());
        return false;
    }

    SDL_SetWindowMinimumSize(mWindow, 640, 480);

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
    AddZipedModDefinitionsFrom(strPath);
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

void Engine::AddZipedModDefinitionsFrom(std::string path)
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
    DataPaths dataPaths(*mFileSystem,
        "{GameDir}/data/DataSetIds.json",
        "{UserDir}/DataSets.json");

    ResourceMapper mapper(*mFileSystem,
        "{GameDir}/data/dataset_contents.json",
        "{GameDir}/data/animations.json",
        "{GameDir}/data/sounds.json",
        "{GameDir}/data/paths.json",
        "{GameDir}/data/fmvs.json");

    mResourceLocator = std::make_unique<ResourceLocator>(std::move(mapper), std::move(dataPaths));

    // TODO: After user selects game def then add/validate the required paths/data sets in the res mapper
    // also add in any extra maps for resources defined by the mod @ game selection screen
}
