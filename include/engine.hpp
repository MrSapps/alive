#pragma once

#include "gamedata.hpp"
#include "filesystem.hpp"
#include <memory>
#include "oddlib/masher.hpp"
#include "SDL.h"
#include <GL/gl3w.h>
#include "core/audiobuffer.hpp"
#include "resourcemapper.hpp"

class IEngineStateChanger
{
public:
    virtual ~IEngineStateChanger() = default;
    virtual void ToState(std::unique_ptr<class EngineState> state) = 0;
};

class EngineState
{
public:
    EngineState(const EngineState&) = delete;
    EngineState& operator = (const EngineState&) = delete;
    EngineState(IEngineStateChanger& stateChanger) : mStateChanger(stateChanger)  { }
    virtual ~EngineState() = default;
    virtual void Update() = 0;
    virtual void Render(int w, int h, class Renderer& renderer) = 0;
protected:
    IEngineStateChanger& mStateChanger;
};

class Engine : public IEngineStateChanger
{
public:
    Engine();
    virtual ~Engine();
    virtual bool Init();
    virtual int Run();

    virtual void ToState(std::unique_ptr<EngineState> state) override
    {
        mCurrentState = std::move(state);
    }
private:
    void Update();
    void Render();
    bool InitSDL();
    int LoadNanoVgFonts(struct NVGcontext* vg);
    void InitResources();
    void InitGL();
    void RenderVideoUi();
protected:
    virtual void InitSubSystems();
    virtual void DebugRender() { };

    // Audio must init early
    SdlAudioWrapper mAudioHandler;

    FileSystem mFileSystem_old;
    std::unique_ptr<class IFileSystem> mFileSystem;
    GameData mGameData;
 
    SDL_Window* mWindow = nullptr;
    SDL_GLContext mContext = nullptr;

    std::unique_ptr<class ResourceLocator> mResourceLocator;
    std::unique_ptr<class Renderer> mRenderer;
    std::unique_ptr<class Fmv> mFmv;
    std::unique_ptr<class Sound> mSound;
    std::unique_ptr<class Level> mLevel;
    struct GuiContext *mGui = nullptr;

    std::unique_ptr<EngineState> mCurrentState;
    std::vector<GameDefinition> mGameDefinitions;
};
