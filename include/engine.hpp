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

    virtual void Input(const InputState& input) = 0;
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

class InputState final
{
public:
    struct InputItemState final
    {
    public:
        // True for the frame that the key was first pressed down
        // false after that.
        bool mIsPressed = false;

        // True for the frame that key went from mIsDown == true to mIsDown == false
        bool mIsReleased = false;

        // True the whole the they key is held down, false the reset of the time
        bool mIsDown = false;

        void UpdateState()
        {
            if (mRawDownState)
            {
                if (!mIsPressed && !mIsDown)
                {
                    mIsReleased = false;
                    mIsPressed = true;
                    mIsDown = true;
                }
                else if (mIsDown)
                {
                    mIsPressed = false;
                }
            }
            else
            {
                // First time the button has gone up after it was down
                if (mIsPressed || mIsDown)
                {
                    mIsReleased = true;
                }
                // Second time or later we've seen the button is up after it was down
                else
                {
                    mIsReleased = false;
                }
                mIsPressed = false;
                mIsDown = false;
            }
        }
    private:
        // Private raw input state used to calculate mIsPressed and mIsDown, set 
        // on response to key input events
        bool mRawDownState = false;

        // So InputState can set mRawDownState
        friend class InputState;
    };

    void Update()
    {
        // Update set set outside of polling loop
        for (InputState::InputItemState& k : mKeys)
        {
            k.UpdateState();
        }

        for (InputState::InputItemState& k : mMouseButtons)
        {
            k.UpdateState();
        }

        for (auto& controller : mControllers)
        {
            controller.second->Update();
        }
    }

    void OnKeyEvent(const SDL_KeyboardEvent& event)
    {
        // Update state from polling loop
        const auto keyCode = event.keysym.scancode;
        mKeys->mRawDownState = (event.type == SDL_KEYDOWN);
    }

    void OnMouseButtonEvent(const SDL_MouseButtonEvent& event)
    {
        // Update state from polling loop
        if (event.button == SDL_BUTTON_LEFT)
        {
            mMouseButtons[0].mRawDownState = (event.type == SDL_MOUSEBUTTONDOWN);
        }
        else if (event.button == SDL_BUTTON_RIGHT)
        {
            mMouseButtons[1].mRawDownState = (event.type == SDL_MOUSEBUTTONDOWN);
        }
    }

    void AddControllers()
    {
        // Add all of the controllers that are currently plugged in
        for (s32 i = 0; i < SDL_NumJoysticks(); i++) 
        {
            LOG_INFO("Adding controller (static)");
            AddController(i);
        }
    }

    void AddController(const SDL_ControllerDeviceEvent& event)
    {
        LOG_INFO("Adding controller (dynamic)");
        AddController(event.which);
    }

    void RemoveController(const SDL_ControllerDeviceEvent& event)
    {
        LOG_INFO("Removing controller");
        auto it = mControllers.find(event.which);
        if (it != std::end(mControllers))
        {
            mControllers.erase(it);
        }
    }

    void OnControllerButton(const SDL_ControllerButtonEvent& event)
    {
        auto it = mControllers.find(event.which);
        if (it != std::end(mControllers))
        {
            it->second->OnControllerButton(event);
        }
    }

    void OnControllerAxis(const SDL_ControllerAxisEvent& event)
    {
        auto it = mControllers.find(event.which);
        if (it != std::end(mControllers))
        {
            it->second->OnControllerAxis(event);
        }
    }

    struct InputPosition
    {
        s32 mX = 0;
        s32 mY = 0;
    };

    // Index into using SDL_SCANCODE_XYZ e.g SDL_SCANCODE_D etc
    InputItemState mKeys[SDL_NUM_SCANCODES];
    InputItemState mMouseButtons[2];
    InputPosition mMousePosition;

    class Controller
    {
    public:
        Controller(const Controller&) = delete;
        Controller& operator = (const Controller&) = delete;

        Controller(SDL_GameController* controller, SDL_Joystick* joyStick, SDL_JoystickID id)
            : mController(controller), mId(id)
        {
            TRACE_ENTRYEXIT;
            if (SDL_JoystickIsHaptic(joyStick))
            {
                mHaptic = SDL_HapticOpenFromJoystick(joyStick);
                LOG_INFO("Haptic Effects: " << SDL_HapticNumEffects(mHaptic));
                LOG_INFO("Haptic Query: " << SDL_HapticQuery(mHaptic));
                if (SDL_HapticRumbleSupported(mHaptic))
                {
                    if (SDL_HapticRumbleInit(mHaptic) != 0)
                    {
                        LOG_ERROR("Haptic Rumble Init failed: " << SDL_GetError());
                        HapticClose();
                    }
                }
                else 
                {
                    HapticClose();
                }
            }
        }

        ~Controller()
        {
            TRACE_ENTRYEXIT;
            SDL_GameControllerClose(mController);
            HapticClose();
        }

        void OnControllerButton(const SDL_ControllerButtonEvent& event)
        {
            mGamePadButtons[event.button].mRawDownState = (event.type == SDL_CONTROLLERBUTTONDOWN);
        }

        void OnControllerAxis(const SDL_ControllerAxisEvent&)
        {
            LOG_INFO("TODO: OnControllerAxis");

            // TODO: Update mGamePadAxes
        }

        void Update()
        {
            for (InputState::InputItemState& k : mGamePadButtons)
            {
                k.UpdateState();
            }
        }

    private:
        void HapticClose()
        {
            if (mHaptic)
            {
                SDL_HapticClose(mHaptic);
                mHaptic = nullptr;
            }
        }

        SDL_GameController* mController = nullptr;
        SDL_JoystickID mId = 0;
        SDL_Haptic* mHaptic = nullptr;

        // Index into using SDL_CONTROLLER_*
        InputItemState mGamePadButtons[SDL_CONTROLLER_BUTTON_MAX];

        InputPosition mGamePadAxes[SDL_CONTROLLER_AXIS_MAX];
    };

private:
    void AddController(s32 i)
    {
        if (SDL_IsGameController(i))
        {
            SDL_GameController* controller = SDL_GameControllerOpen(i);
            if (controller)
            {
                SDL_Joystick* joyStick = SDL_GameControllerGetJoystick(controller);
                const SDL_JoystickID id = SDL_JoystickInstanceID(joyStick);
                mControllers[id] = std::make_unique<Controller>(controller, joyStick, id);
            }
            else
            {
                LOG_ERROR("Failed to open game controller " << i << " due to error: " << SDL_GetError());
            }
        }
    }

    std::map<u32, std::unique_ptr<Controller>> mControllers;
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
