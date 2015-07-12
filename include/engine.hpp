#pragma once

#include "gamedata.hpp"
#include "filesystem.hpp"
#include "fmv.hpp"
#include <memory>
#include "oddlib/masher.hpp"
#include <SDL.h>

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
private:
    std::string mBasePath;
    bool mRunning = true;
    FileSystem mFileSystem;
    GameData mGameData;
    Fmv mFmv;

    // temp
    std::unique_ptr<Oddlib::Masher> video;
    SDL_Surface* videoFrame = NULL;
};
