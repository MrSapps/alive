#pragma once

#include "types.hpp"
#include "bitutils.hpp"
#include <map>
#include <vector>
#include <memory>
#include "logger.hpp"
#include <SDL.h>

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


class InputReader;

class InputMapping
{
public:
    void Update(const InputReader& input);

    InputMapping();

    // TODO: Allow remapping/read from config file
    struct KeyBoardInputConfig
    {
        u32 mNumberOfKeysRequired;
        std::vector<SDL_Scancode> mButtonsRequired;
    };

    struct GamePadInputConfig
    {
        u32 mNumberOfKeysRequired;
        std::vector<SDL_GameControllerButton> mButtonsRequired;
    };

    u32 Pressed() const { return ~mOldPressedState & mCurrentPressedState; }
    u32 Released() const { return mOldPressedState & ~mCurrentPressedState; }
    u32 Held() const { return mOldPressedState & mCurrentPressedState; }
    u32 PressedOrHeld() const { return Pressed() | Held(); }

    u32 Pressed(u32 mask) const { return Pressed() & mask; }
    u32 Released(u32 mask) const { return Released() & mask; }
    u32 Held(u32 mask) const { return Held() & mask; }
    u32 PressedOrHeld(u32 mask) const { return PressedOrHeld() & mask; }
private:


    u32 GetNewPressedState(const InputReader& input);

    std::map<InputCommands, KeyBoardInputConfig> mKeyBoardConfig;
    std::map<InputCommands, GamePadInputConfig> mGamePadConfig;

    u32 mOldPressedState = 0;
    u32 mCurrentPressedState = 0;
};

class Controller
{
public:
    Controller(const Controller&) = delete;
    Controller& operator = (const Controller&) = delete;
    Controller(SDL_GameController* controller, SDL_Joystick* joyStick);
    ~Controller();
    void OnControllerButton(const SDL_ControllerButtonEvent& event);
    void OnControllerAxis(const SDL_ControllerAxisEvent& event);
    void Rumble(f32 strength);

    // Index into using SDL_CONTROLLER_*
    bool mGamePadButtonsPressed[SDL_CONTROLLER_BUTTON_MAX] = {};
    f32 mGamePadAxes[SDL_CONTROLLER_AXIS_MAX] = {};
private:
    void HapticClose();

    SDL_GameController* mController = nullptr;
    SDL_Haptic* mHaptic = nullptr;
};

class ButtonState
{
public:
    void OnPressed(bool bPressed)
    {
        mBufferedPressedState = bPressed;
    }

    void Update()
    {
        mOldPressedState = mCurrentPressedState;
        mCurrentPressedState = mBufferedPressedState;
    }

    bool Pressed() const { return !mOldPressedState && mCurrentPressedState; }
    bool Released() const { return mOldPressedState && !mCurrentPressedState; }
    bool Held() const { return mOldPressedState && mCurrentPressedState; }
    bool PressedOrHeld() const { return Pressed() || Held(); }
private:
    bool mOldPressedState = false;
    bool mCurrentPressedState = false;
    bool mBufferedPressedState = false;

};

enum class MouseButtons
{
    eLeft = 0,
    eRight = 1,
};

class InputReader final
{
public:
    void Update();

    ~InputReader();
    void OnKeyEvent(const SDL_KeyboardEvent& event);
    void OnMouseButtonEvent(const SDL_MouseButtonEvent& event);
    
    void AddControllers();
    void AddController(const SDL_ControllerDeviceEvent& event);
    void RemoveController(const SDL_ControllerDeviceEvent& event);
    void OnControllerButton(const SDL_ControllerButtonEvent& event);
    void OnControllerAxis(const SDL_ControllerAxisEvent& event);
    const Controller* ActiveController() const;

    const ButtonState& KeyboardKey(SDL_Scancode scanCode) const
    {
        return mKeyboardKeys[scanCode];
    }

    const ButtonState& MouseButton(MouseButtons button) const
    {
        return mMouseButtons[static_cast<u32>(button)];
    }

    struct MousePosition
    {
        s32 mX = 0;
        s32 mY = 0;
    };

    const MousePosition& MousePos() const
    {
        return mMousePosition;
    }

    const InputMapping& GameCommands() const { return mMapping; }
    void SetMousePos(s32 x, s32 y);
private:
    void AddController(s32 index);

    ButtonState mKeyboardKeys[SDL_NUM_SCANCODES];
    ButtonState mMouseButtons[2] = {};
    MousePosition mMousePosition;

    // Currently connected controllers to query for reading.
    std::map<u32, std::unique_ptr<Controller>> mControllers;

    InputMapping mMapping;
};
