#pragma once

#include <memory>
#include "oddlib/masher.hpp"
#include "SDL.h"
#include <GL/gl3w.h>
#include "core/audiobuffer.hpp"
#include "resourcemapper.hpp"

class StateMachine;
class InputState;

class IState
{
public:
    IState(StateMachine& stateMachine) : mStateMachine(stateMachine) {}
    IState(const IState&) = delete;
    IState& operator = (const IState&) = delete;
    virtual ~IState() = default;

    virtual void Input(InputState& input) = 0;
    virtual void Update() = 0;
    virtual void Render(int w, int h, class Renderer& renderer) = 0;
    virtual void ExitState() = 0;
    virtual void EnterState() = 0;
protected:
    StateMachine& mStateMachine;
};

class StateMachine
{
public:
    virtual ~StateMachine() = default;

    void ToState(std::unique_ptr<IState> state)
    {
        if (mState)
        {
            mState->ExitState();
        }

        // Delay delete of mState until the next Update() to prevent
        // a "delete this" happening during a call to ToState().
        mPreviousState = std::move(mState);
        mState = std::move(state);

        if (mState)
        {
            mState->EnterState();
        }
    }

    void Input(class InputState& input)
    {
        if (mState)
        {
            mState->Input(input);
        }
    }

    void Update()
    {
        if (mState)
        {
            mState->Update();
        }

        // Destroy previous state one update later
        if (mPreviousState)
        {
            mPreviousState = nullptr;
        }
    }

    void Render(int w, int h, class Renderer& renderer)
    {
        if (mState)
        {
            mState->Render(w, h, renderer);
        }
    }

    bool HasState() const { return mState != nullptr; }
private:
    std::unique_ptr<class IState> mState = nullptr;
    std::unique_ptr<class IState> mPreviousState = nullptr;
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
    u32 mLeftMouseState = 0;
    u32 mRightMouseState = 0;

    s32 mMouseX = 0;
    s32 mMouseY = 0;
};

class Engine final
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
    int LoadNanoVgFonts(struct NVGcontext* vg);
    void AddGameDefinitionsFrom(const char* path);
    void AddModDefinitionsFrom(const char* path);
    void AddDirectoryBasedModDefinitionsFrom(std::string path);
    void AddZipsedModDefinitionsFrom(std::string path);
    void InitResources();
    void InitGL();
    void RenderVideoUi();
protected:
    void InitSubSystems();

    // Audio must init early
    SdlAudioWrapper mAudioHandler;

    std::unique_ptr<class IFileSystem> mFileSystem;
 
    SDL_Window* mWindow = nullptr;
    SDL_GLContext mContext = nullptr;

    std::unique_ptr<class ResourceLocator> mResourceLocator;
    std::unique_ptr<class Renderer> mRenderer;
    std::unique_ptr<class Fmv> mFmv;
    std::unique_ptr<class Sound> mSound;
    std::unique_ptr<class Level> mLevel;
    struct GuiContext *mGui = nullptr;

    std::vector<GameDefinition> mGameDefinitions;

    InputState mInputState;

    StateMachine mStateMachine;
};
