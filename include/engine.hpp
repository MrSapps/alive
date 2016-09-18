#pragma once

#include <memory>
#include "oddlib/masher.hpp"
#include "SDL.h"
#include <GL/gl3w.h>
#include "core/audiobuffer.hpp"
#include "resourcemapper.hpp"
#include "proxy_sol.hpp"

class StateMachine;
class InputState;

class IState
{
public:
    IState(StateMachine& stateMachine) : mStateMachine(stateMachine) {}
    IState(const IState&) = delete;
    IState& operator = (const IState&) = delete;
    virtual ~IState() = default;

    virtual void Update(const InputState& input) = 0;
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

    void Update(const InputState& input)
    {
        if (mState)
        {
            mState->Update(input);
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

class Actions
{
public:
    Actions() = default;
    bool InputLeft() const { return mActions[Left].mIsDown; }
    bool InputRight() const { return mActions[Right].mIsDown; }
    bool InputUp() const { return mActions[Up].mIsDown; }
    bool InputDown() const { return mActions[Down].mIsDown; }
    bool InputChant() const { return mActions[Chant].mIsDown; }
    bool InputRun() const { return mActions[Run].mIsDown; }
    bool InputSneak() const { return mActions[Sneak].mIsDown; }
    bool InputJump() const { return mActions[Jump].mIsDown; }
    bool InputThrow() const { return mActions[Throw].mIsDown; }
    bool InputAction() const { return mActions[Action].mIsDown; }
    bool InputRollOrFart() const { return mActions[RollOrFart].mIsDown; }
    bool InputGameSpeak1() const { return mActions[GameSpeak1].mIsDown; }
    bool InputGameSpeak2() const { return mActions[GameSpeak2].mIsDown; }
    bool InputGameSpeak3() const { return mActions[GameSpeak3].mIsDown; }
    bool InputGameSpeak4() const { return mActions[GameSpeak4].mIsDown; }
    bool InputGameSpeak5() const { return mActions[GameSpeak5].mIsDown; }
    bool InputGameSpeak6() const { return mActions[GameSpeak6].mIsDown; }
    bool InputGameSpeak7() const { return mActions[GameSpeak7].mIsDown; }
    bool InputGameSpeak8() const { return mActions[GameSpeak8].mIsDown; }
    bool InputBack() const { return mActions[Back].mIsDown; }

    static void RegisterLuaBindings(sol::state& state);

    enum EInputActions
    {
        Left,       // Arrow left
        Right,      // Arrow right
        Up,         // Arrow up
        Down,       // Arrow down
        Chant,      // 0
        Run,        // Shift
        Sneak,      // Alt
        Jump,       // Space
        Throw,      // Z
        Action,     // Control
        RollOrFart, // X (Only roll)
        GameSpeak1, // 1 (Hello)
        GameSpeak2, // 2 (Follow Me)
        GameSpeak3, // 3 (Wait)
        GameSpeak4, // 4 (Work) (Whistle 1)
        GameSpeak5, // 5 (Anger)
        GameSpeak6, // 6 (All ya) (Fart)
        GameSpeak7, // 7 (Sympathy) (Whistle 2)
        GameSpeak8, // 8 (Stop it) (Laugh)
        Back,       // Esc
        Max
    };
    InputItemState mActions[Max];
};

class InputMapping
{
public:
    void Update(const InputState& input);

    InputMapping();

    // TODO: Allow remapping/read from config file
    struct KeyBoardInputConfig
    {
        u32 mNumberOfKeysRequired;
        std::vector<SDL_Scancode> mKeys;
    };

    struct GamePadInputConfig
    {
        u32 mNumberOfKeysRequired;
        std::vector<SDL_GameControllerButton> mButtons;
    };

    std::map<Actions::EInputActions, KeyBoardInputConfig> mKeyBoardConfig;
    std::map<Actions::EInputActions, GamePadInputConfig> mGamePadConfig;

    const Actions& GetActions() const { return mActions; }

private:
    Actions mActions;
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
                //Rumble(1.0f);
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

    sol::state mLuaState;
};
