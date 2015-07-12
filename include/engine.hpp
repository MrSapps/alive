#pragma once

#include "gamedata.hpp"
#include "filesystem.hpp"
#include "fmv.hpp"
#include <memory>
#include "oddlib/masher.hpp"
#include "SDL.h"
//#include <GL/glew.h>
#ifdef _WIN32
#include <windows.h>
#endif
#ifdef __APPLE__
#  include <OpenGL/gl.h>
#else
#  include <GL/gl.h>
#endif/*__APPLE__*/

class Engine
{
public:
    Engine();
    ~Engine();
    bool Init();
    int Run();
private:
    void Update();
    void Render();
    bool InitSDL();
    void InitGL();
    void InitImGui();
    void InitBasePath();
    void RenderVideoUi();
    void ImGui_WindowResize();
private:
    std::string mBasePath;
    bool mRunning = true;
    FileSystem mFileSystem;
    GameData mGameData;
    Fmv mFmv;

    // temp
    std::unique_ptr<Oddlib::Masher> video;
    SDL_Surface* videoFrame = NULL;
    SDL_Window* window;
    SDL_GLContext context;
};
