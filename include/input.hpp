#pragma once

#include "types.hpp"
#include "bitutils.hpp"
#include <map>
#include <vector>
#include <memory>
#include "logger.hpp"
#include <SDL.h>

template<class T>
void UpdateStateInputItemGeneric(T& state)
{
    if (state.RawDownState())
    {
        if (!state.IsPressed() && !state.IsDown())
        {
            state.SetIsReleased(false);
            state.SetIsPressed(true);
            state.SetIsDown(true);
        }
        else if (state.IsDown())
        {
            state.SetIsPressed(false);
            state.SetIsReleased(false);
        }
    }
    else
    {
        // First time the button has gone up after it was down
        if (state.IsPressed() || state.IsDown())
        {
            state.SetIsReleased(true);
        }
        // Second time or later we've seen the button is up after it was down
        else
        {
            state.SetIsReleased(false);
        }
        state.SetIsPressed(false);
        state.SetIsDown(false);
    }
}


enum PsxButtonBits : u32
{
    eL2 =           1 << 0,
    eR2 =           1 << 1,
    eL1 =           1 << 2,
    eR1 =           1 << 3,
    eTriangle =     1 << 4,
    eCircle =       1 << 5,
    eCross =        1 << 6,
    eSquare =       1 << 7,
    eSelect =       1 << 8,
    // As seen in LibEtc.h of PSYQ.. don't think these can ever be used.
    // PADi 9 ?
    // PADj 10 ?
    eStart =        1 << 11,
    eDPadUp =       1 << 12,
    eDPadRight =    1 << 13,
    eDPadDown =     1 << 14,
    eDPadLeft =     1 << 15,
};

enum InputCommands : u32
{
    eUp =           1 << 0,
    eDown =         1 << 1,
    eLeft =         1 << 2,
    eRight =        1 << 3,
    eRun =          1 << 4,
    eDoAction =     1 << 5,  // Pick up rock, pull lever etc
    eSneak =        1 << 6,
    eThrowItem =    1 << 7,  // Or I say I dunno if no items
    eHop =          1 << 8,
    eFartOrRoll =   1 << 9,  // (Only roll in AO)
    eGameSpeak1 =   1 << 10, // Hello
    eGameSpeak2 =   1 << 11, // (Follow Me)
    eGameSpeak3 =   1 << 12, // Wait
    eGameSpeak4 =   1 << 13, // (Work) (Whistle 1)
    eGameSpeak5 =   1 << 14, // (Anger)
    eGameSpeak6 =   1 << 15, // (All ya) (Fart)
    eGameSpeak7 =   1 << 16, // (Sympathy) (Whistle 2)
    eGameSpeak8 =   1 << 17, // (Stop it) (Laugh)
    eChant =        1 << 18, 
    ePause =        1 << 19, // Or enter
    eUnPause =      1 << 20, // Or/and back
    // 0x200000     = nothing
    eCheatMode =    1 << 22,
    // 0x800000     = nothing
    // 0x1000000    = nothing
    // 0x2000000    = nothing
    // 0x4000000    = nothing
    // 0x8000000    = nothing
    // 0x10000000   = nothing
    // 0x20000000   = nothing
    // 0x40000000   = nothing
    // 0x80000000   = nothing
};

struct InputItemState final
{
private:
    // True for the frame that the key was first pressed down
    // false after that.
    bool mIsPressed = false;

    // True for the frame that key went from mIsDown == true to mIsDown == false
    bool mIsReleased = false;

    // True the whole the they key is held down, false the reset of the time
    bool mIsDown = false;

    // Raw input state used to calculate mIsPressed and mIsDown, set 
    // on response to key input events
    bool mRawDownState = false;
public:
    void SetIsPressed(bool val) { mIsPressed = val; }
    void SetIsReleased(bool val) { mIsReleased = val; }
    void SetIsDown(bool val) { mIsDown = val; }
    void SetRawDownState(bool val) { mRawDownState = val; }

    bool IsPressed() const { return mIsPressed; }
    bool IsReleased() const { return mIsReleased; }
    bool IsDown() const { return mIsDown; }
    bool RawDownState() const { return mRawDownState; }

    void UpdateState()
    {
        ::UpdateStateInputItemGeneric(*this);
    }
};

class Actions
{
public:
    Actions()
    {
        TRACE_ENTRYEXIT;
    }

    static bool Left(u32 state) { return IsBitOn(state, eLeft); }
    static bool Right(u32 state) { return IsBitOn(state, eRight); }
    static bool Up(u32 state) { return IsBitOn(state, eUp); }
    static bool Down(u32 state) { return IsBitOn(state, eDown); }
    static bool Chant(u32 state) { return IsBitOn(state, eChant); }
    static bool Run(u32 state) { return IsBitOn(state, eRun); }
    static bool Sneak(u32 state) { return IsBitOn(state, eSneak); }
    static bool Jump(u32 state) { return IsBitOn(state, eJump); }
    static bool Throw(u32 state) { return IsBitOn(state, eThrow); }
    static bool Action(u32 state) { return IsBitOn(state, eAction); }
    static bool RollOrFart(u32 state) { return IsBitOn(state, eRollOrFart); }
    static bool GameSpeak1(u32 state) { return IsBitOn(state, eGameSpeak1); }
    static bool GameSpeak2(u32 state) { return IsBitOn(state, eGameSpeak2); }
    static bool GameSpeak3(u32 state) { return IsBitOn(state, eGameSpeak3); }
    static bool GameSpeak4(u32 state) { return IsBitOn(state, eGameSpeak4); }
    static bool GameSpeak5(u32 state) { return IsBitOn(state, eGameSpeak5); }
    static bool GameSpeak6(u32 state) { return IsBitOn(state, eGameSpeak6); }
    static bool GameSpeak7(u32 state) { return IsBitOn(state, eGameSpeak7); }
    static bool GameSpeak8(u32 state) { return IsBitOn(state, eGameSpeak8); }
    static bool Back(u32 state) { return IsBitOn(state, eBack); }


    enum EInputActions : u32
    {
        eLeft = 1,   // Arrow left
        eRight = 2,   // Arrow right
        eUp = 3,   // Arrow up
        eDown = 4,   // Arrow down
        eChant = 5,   // 0
        eRun = 6,   // Shift
        eSneak = 7,   // Alt
        eJump = 8,   // Space
        eThrow = 9,   // Z
        eAction = 10,  // Control
        eRollOrFart = 11,  // X (Only roll)
        eGameSpeak1 = 12,  // 1 (Hello)
        eGameSpeak2 = 13,  // 2 (Follow Me)
        eGameSpeak3 = 14,  // 3 (Wait)
        eGameSpeak4 = 15,  // 4 (Work) (Whistle 1)
        eGameSpeak5 = 16,  // 5 (Anger)
        eGameSpeak6 = 17,  // 6 (All ya) (Fart)
        eGameSpeak7 = 18,  // 7 (Sympathy) (Whistle 2)
        eGameSpeak8 = 19,  // 8 (Stop it) (Laugh)
        eBack = 20,  // Esc
        Max = 21
    };

    // Same as InputItemState, but we collapse the bools into bitflags
    // since we have less than 32bits of input. This also makes the scriping
    // faster as we often need to save all of the old pressed states which becomes
    // a simple u32 copy.

    // So we can call UpdateStateInputItemGeneric which will then call
    // InputStateAdaptor::SetIsPressed(state) which calls the outter SetIsPressed(action,state)
    struct InputStateAdaptor
    {
        Actions& mParent;
        EInputActions mAction;

        InputStateAdaptor(Actions& parent, EInputActions action) : mParent(parent), mAction(action) { }
        InputStateAdaptor(InputStateAdaptor&&) = delete;
        InputStateAdaptor& operator = (InputStateAdaptor&&) = delete;
        void SetIsPressed(bool val) { mParent.SetIsPressed(mAction, val); }
        void SetIsReleased(bool val) { mParent.SetIsReleased(mAction, val); }
        void SetIsDown(bool val) { mParent.SetIsDown(mAction, val); }
        void SetRawDownState(bool val) { mParent.SetRawDownState(mAction, val); }

        bool IsPressed() const { return mParent.IsPressed(mAction); }
        bool IsReleased() const { return mParent.IsReleased(mAction); }
        bool IsDown() const { return mParent.IsDown(mAction); }
        bool RawDownState() const { return mParent.RawDownState(mAction); }
    };

    void SetIsPressed(EInputActions action, bool val) { BitOnOrOff(mIsPressed, action, val); }
    void SetIsReleased(EInputActions action, bool val) { BitOnOrOff(mIsReleased, action, val); }
    void SetIsDown(EInputActions action, bool val) { BitOnOrOff(mIsDown, action, val); }
    void SetRawDownState(EInputActions action, bool val) { BitOnOrOff(mRawDownState, action, val); }

    bool IsPressed(EInputActions action) const { return  IsBitOn(mIsPressed, action); }
    bool IsReleased(EInputActions action) const { return IsBitOn(mIsReleased, action); }
    bool IsDown(EInputActions action) const { return IsBitOn(mIsDown, action); }
    bool RawDownState(EInputActions action) const { return IsBitOn(mRawDownState, action); }

    void UpdateStates()
    {
        for (u32 i = eLeft; i < Max; i++)
        {
            InputStateAdaptor adaptor(*this, static_cast<EInputActions>(i));
            UpdateStateInputItemGeneric(adaptor);
        }
    }

    u32 mIsPressed = 0;
    u32 mIsReleased = 0;
    u32 mIsDown = 0;
    u32 mRawDownState = 0;
};

class InputState;

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
    ~InputState()
    {
        for (auto& c : mControllers)
        {
            c.second.release();
        }
    }

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
        mKeys[keyCode].SetRawDownState(event.type == SDL_KEYDOWN);
    }

    void OnMouseButtonEvent(const SDL_MouseButtonEvent& event)
    {
        // Update state from polling loop
        if (event.button == SDL_BUTTON_LEFT)
        {
            mMouseButtons[0].SetRawDownState(event.type == SDL_MOUSEBUTTONDOWN);
        }
        else if (event.button == SDL_BUTTON_RIGHT)
        {
            mMouseButtons[1].SetRawDownState(event.type == SDL_MOUSEBUTTONDOWN);
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
            mGamePadButtons[event.button].SetRawDownState(event.type == SDL_CONTROLLERBUTTONDOWN);
            if (mGamePadButtons[event.button].RawDownState())
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
