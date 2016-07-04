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

class InputState
{
public:
    enum EPressedState
    {
        eHeld = 0x1,
        eDown = 0x2,
        eUp = 0x4,
    };
    Uint32 mLeftMouseState = 0;
    Uint32 mRightMouseState = 0;

    Sint32 mMouseX = 0;
    Sint32 mMouseY = 0;
};

class EngineState
{
public:
    EngineState(const EngineState&) = delete;
    EngineState& operator = (const EngineState&) = delete;
    EngineState(IEngineStateChanger& stateChanger) : mStateChanger(stateChanger)  { }
    virtual ~EngineState() = default;
    virtual void Input(InputState& input) = 0;
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
        if (!state)
        {
            mCurrentState = nullptr;
            mNextState = nullptr;
        }
        else
        {
            if (!mCurrentState)
            {
                mCurrentState = std::move(state);
            }
            else
            {
                mNextState = std::move(state);
            }
        }
    }
private:
    void Update();
    void Render();
    bool InitSDL();
    int LoadNanoVgFonts(struct NVGcontext* vg);
    void AddGameDefinitionsFrom(const char* path);
    void AddModDefinitionsFrom(const char* path);
    void AddDirectoryBasedModDefinitionsFrom(std::string path);
    void AddZipsedModDefinitionsFrom(std::string path);
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
    std::unique_ptr<EngineState> mNextState;
    std::vector<GameDefinition> mGameDefinitions;

    InputState mInputState;
};
