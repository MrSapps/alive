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
                mIsReleased = false;
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

    // Private raw input state used to calculate mIsPressed and mIsDown, set 
    // on response to key input events
    bool mRawDownState = false;
};

class InputMapping
{
public:
    enum EPsxButtons
    {
        Cross,
        Triangle,
        Circle,
        Square,
        Left,
        Right,
        Up,
        Down,
        L1,
        L2,
        R1,
        R2,
        Start,
        Select,
        Max
    };

    void Update(const InputState& input);

    InputMapping()
    {
        // TODO: Set up default key mappings
        mKeyBoardConfig[SDL_SCANCODE_LEFT] = Left;
        mKeyBoardConfig[SDL_SCANCODE_RIGHT] = Right;
        mKeyBoardConfig[SDL_SCANCODE_UP] = Up;
        mKeyBoardConfig[SDL_SCANCODE_DOWN] = Down;

        mGamePadConfig[SDL_CONTROLLER_BUTTON_DPAD_LEFT] = Left;
        mGamePadConfig[SDL_CONTROLLER_BUTTON_DPAD_RIGHT] = Right;
        mGamePadConfig[SDL_CONTROLLER_BUTTON_DPAD_UP] = Up;
        mGamePadConfig[SDL_CONTROLLER_BUTTON_DPAD_DOWN] = Down;

    }

    InputItemState mButtons[Max];

    // TODO: Allow remapping/read from config file
    std::map<SDL_Scancode, EPsxButtons> mKeyBoardConfig;
    std::map<SDL_GameControllerButton, EPsxButtons> mGamePadConfig;
};

class InputState final
{
public:
    void Update()
    {
        // Update set set outside of polling loop
        for (InputItemState& inputItem : mKeys)
        {
            inputItem.UpdateState();
        }

        for (InputItemState& inputItem : mMouseButtons)
        {
            inputItem.UpdateState();
        }

        for (auto& controller : mControllers)
        {
            controller.second->Update();
        }
        mInputMapping.Update(*this);
    }

    void OnKeyEvent(const SDL_KeyboardEvent& event)
    {
        // Update state from polling loop
        const u32 keyCode = event.keysym.scancode;
        mKeys[keyCode].mRawDownState = (event.type == SDL_KEYDOWN);
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
        auto it = mControllers.find(event.which);
        if (it == std::end(mControllers))
        {
            AddController(event.which);
        }
        else
        {
            LOG_WARNING("Controlled already added (statically?)");
        }
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

        Controller(SDL_GameController* controller, SDL_Joystick* joyStick)
            : mController(controller)
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
            HapticClose();
            SDL_GameControllerClose(mController);
        }

        void OnControllerButton(const SDL_ControllerButtonEvent& event)
        {
            mGamePadButtons[event.button].mRawDownState = (event.type == SDL_CONTROLLERBUTTONDOWN);
            if (mGamePadButtons[event.button].mRawDownState)
            {
                Rumble(1.0f);
            }
        }

        void OnControllerAxis(const SDL_ControllerAxisEvent& event)
        {
            mGamePadAxes[event.axis] = event.value;
            //Rumble(1.0f);
        }

        void Update()
        {
            for (InputItemState& inputItem : mGamePadButtons)
            {
                inputItem.UpdateState();
            }
        }

        void Rumble(f32 strength)
        {
            if (mHaptic)
            {
                if (SDL_HapticRumblePlay(mHaptic, strength, 300) != 0)
                {
                    LOG_ERROR("SDL_HapticRumblePlay: " << SDL_GetError());
                }
            }
        }

        // Index into using SDL_CONTROLLER_*
        InputItemState mGamePadButtons[SDL_CONTROLLER_BUTTON_MAX];
        f32 mGamePadAxes[SDL_CONTROLLER_AXIS_MAX] = {};
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
        SDL_Haptic* mHaptic = nullptr;
    };

    const Controller* ActiveController() const
    {
        if (mControllers.empty())
        {
            return nullptr;
        }
        return mControllers.begin()->second.get();
    }

    const InputMapping& Mapping() const { return mInputMapping; }
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
                mControllers[id] = std::make_unique<Controller>(controller, joyStick);
            }
            else
            {
                LOG_ERROR("Failed to open game controller " << i << " due to error: " << SDL_GetError());
            }
        }
    }

    std::map<u32, std::unique_ptr<Controller>> mControllers;
    InputMapping mInputMapping;
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
