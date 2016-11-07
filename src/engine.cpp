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

/*static*/ void Actions::RegisterLuaBindings(sol::state& state)
{
    std::ignore = state;
    state.new_usertype<Actions>("Actions",
        "Left", &Actions::Left,
        "Right", &Actions::Right,
        "Up", &Actions::Up,
        "Down", &Actions::Down,
        "Chant", &Actions::Chant,
        "Run", &Actions::Run,
        "Sneak", &Actions::Sneak,
        "Jump", &Actions::Jump,
        "Throw", &Actions::Throw,
        "Action", &Actions::Action,
        "RollOrFart", &Actions::RollOrFart,
        "GameSpeak1", &Actions::GameSpeak1,
        "GameSpeak2", &Actions::GameSpeak2,
        "GameSpeak3", &Actions::GameSpeak3,
        "GameSpeak4", &Actions::GameSpeak4,
        "GameSpeak5", &Actions::GameSpeak5,
        "GameSpeak6", &Actions::GameSpeak6,
        "GameSpeak7", &Actions::GameSpeak7,
        "GameSpeak8", &Actions::GameSpeak8,
        "Back", &Actions::Back,
        "IsPressed", sol::readonly(&Actions::mIsPressed),
        "IsReleased", sol::readonly(&Actions::mIsReleased),
        "IsHeld", sol::readonly(&Actions::mIsDown)
        );
}

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


        Debugging().mInput = &mInputState;
        
        mStateMachine.ToState(std::make_unique<GameSelectionScreen>(mStateMachine, mGameDefinitions, mGui, *mFmv, *mSound, *mLevel, *mResourceLocator, *mFileSystem));

        return true;
    }
    catch (const std::exception& ex)
    {
        LOG_ERROR("Caught error when trying to init: " << ex.what());
        return false;
    }
}

void LuaLogTrace(const char* msg) { if (msg) { LOG_NOFUNC_TRACE(msg); } else  { LOG_NOFUNC_TRACE("nil"); } }
void LuaLogInfo(const char* msg) { if (msg) { LOG_NOFUNC_INFO(msg); } else { LOG_NOFUNC_INFO("nil"); } }
void LuaLogWarning(const char* msg) { if (msg) { LOG_NOFUNC_WARNING(msg); } else { LOG_NOFUNC_WARNING("nil"); } }
void LuaLogError(const char* msg) { if (msg) { LOG_NOFUNC_ERROR(msg); } else { LOG_NOFUNC_ERROR("nil"); } }

void Engine::InitSubSystems()
{
    mRenderer = std::make_unique<Renderer>((mFileSystem->FsPath() + "data/Roboto-Regular.ttf").c_str());
    mFmv = std::make_unique<DebugFmv>(mAudioHandler, *mResourceLocator);
    mSound = std::make_unique<Sound>(mAudioHandler, *mResourceLocator, mLuaState);
    mLevel = std::make_unique<Level>(mAudioHandler, *mResourceLocator, mLuaState, *mRenderer);

    { // Init gui system
        mGui = create_gui(&calcTextSize, mRenderer.get());
        load_layout(mGui);
    }

    mInputState.AddControllers();

    mLuaState.open_libraries(sol::lib::base, sol::lib::string, sol::lib::jit, sol::lib::table, sol::lib::debug, sol::lib::math, sol::lib::bit32, sol::lib::package);
    
    // Redirect lua print()
    mLuaState.set_function("print", LuaLogTrace);

    // Add other logging globals
    mLuaState.set_function("log_info", LuaLogInfo);
    mLuaState.set_function("log_trace", LuaLogTrace);
    mLuaState.set_function("log_warning", LuaLogWarning);
    mLuaState.set_function("log_error", LuaLogError);

    // Get lua to look for scripts in the correction location
    mLuaState.script("package.path = '" + mFileSystem->FsPath() + "data/scripts/?.lua'");

    Oddlib::IStream::RegisterLuaBindings(mLuaState);
    Actions::RegisterLuaBindings(mLuaState);
    MapObject::RegisterLuaBindings(mLuaState);
    ObjRect::RegisterLuaBindings(mLuaState);
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
            mInputState.OnMouseButtonEvent(event.button);

            int guiKey = -1;

            if (event.button.button == SDL_BUTTON_LEFT)
            {
                guiKey = GUI_KEY_LMB;
            }
            else if (event.button.button == SDL_BUTTON_MIDDLE)
            {
                guiKey = GUI_KEY_MMB;
            }
            else if (event.button.button == SDL_BUTTON_RIGHT)
            {
                guiKey = GUI_KEY_RMB;
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

    // Set rest of gui input state which wasn't set in event polling loop
    SDL_PumpEvents();

    int mouse_x, mouse_y;
    SDL_GetMouseState(&mouse_x, &mouse_y);
    mGui->cursor_pos[0] = mouse_x;
    mGui->cursor_pos[1] = mouse_y;

    mInputState.mMousePosition.mX = mouse_x;
    mInputState.mMousePosition.mY = mouse_y;

    if (SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_LEFT))
    {
        mGui->key_state[GUI_KEY_LMB] |= GUI_KEYSTATE_DOWN_BIT;
    }

    if (SDL_GetKeyboardState(NULL)[SDL_SCANCODE_LCTRL])
    {
        mGui->key_state[GUI_KEY_LCTRL] |= GUI_KEYSTATE_DOWN_BIT;
    }

    mInputState.Update();
    Debugging().Update(mInputState);

    mStateMachine.Update(mInputState, *mRenderer);
    mRenderer->Update();
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

    Debugging().Render(*mRenderer, *mGui);

    gui_post_frame(mGui);

    drawWidgets(*mGui, *mRenderer);
    mRenderer->endFrame();

    SDL_GL_SwapWindow(mWindow);
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

    mWindow = SDL_CreateWindow(WindowTitle(0.0f),
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480,
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

    SDL_GL_SetSwapInterval(1);
    //SDL_GL_SetSwapInterval(0); // No vsync for gui, for responsiveness

    if (gl3wInit()) 
    {
        throw Oddlib::Exception("failed to initialize OpenGL");
    }

    if (!gl3wIsSupported(3, 1)) 
    {
        throw Oddlib::Exception("OpenGL 3.1 not supported");
    }
}
