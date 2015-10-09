#pragma once

#include "gamedata.hpp"
#include "filesystem.hpp"
#include "fmv.hpp"
#include "sound.hpp"
#include <memory>
#include "oddlib/masher.hpp"
#include "core/audiobuffer.hpp"
#include "SDL.h"
#include <GL/glew.h>
#include "SDL_opengl.h"

class Engine
{
public:
    Engine();
    virtual ~Engine();
    virtual bool Init();
    virtual int Run();
private:
    void Update();
    void Render();
    bool InitSDL();
    int LoadNanoVgFonts(struct NVGcontext* vg);
    void InitNanoVg();
    void InitGL();
    void InitImGui();
    void RenderVideoUi();
    void ImGui_WindowResize();
private:
    enum eStates
    {
        eStarting,
        eRunning,
        ePlayingFmv,
        eShuttingDown,
    };
    eStates mPreviousState = eStarting;
    eStates mState = eStarting;
    
    void ToState(eStates newState);

    // Audio must init early
    SdlAudioWrapper mAudioHandler;

    FileSystem mFileSystem;
    GameData mGameData;
 
    SDL_Window* mWindow = nullptr;
    SDL_GLContext mContext = nullptr;


    Fmv mFmv;
    Sound mSound;

    struct NVGLUframebuffer* mNanoVgFrameBuffer = nullptr;
    struct NVGcontext* mNanoVg = nullptr;
};
