#include "input.hpp"


InputMapping::InputMapping()
{
    // Keyboard
    mKeyBoardConfig[InputCommands::eLeft] = { 1u,{ SDL_SCANCODE_LEFT } };
    mKeyBoardConfig[InputCommands::eRight] = { 1u,{ SDL_SCANCODE_RIGHT } };
    mKeyBoardConfig[InputCommands::eUp] = { 1u,{ SDL_SCANCODE_UP } };
    mKeyBoardConfig[InputCommands::eDown] = { 1u,{ SDL_SCANCODE_DOWN } };
    mKeyBoardConfig[InputCommands::eChant] = { 1u,{ SDL_SCANCODE_0 } };
    mKeyBoardConfig[InputCommands::eRun] = { 1u,{ SDL_SCANCODE_LSHIFT,    SDL_SCANCODE_RSHIFT } };
    mKeyBoardConfig[InputCommands::eSneak] = { 1u,{ SDL_SCANCODE_LALT,      SDL_SCANCODE_RALT } };
    mKeyBoardConfig[InputCommands::eHop] = { 1u,{ SDL_SCANCODE_SPACE } };
    mKeyBoardConfig[InputCommands::eThrowItem] = { 1u,{ SDL_SCANCODE_Z } };
    mKeyBoardConfig[InputCommands::eDoAction] = { 1u,{ SDL_SCANCODE_LCTRL,     SDL_SCANCODE_RCTRL } };
    mKeyBoardConfig[InputCommands::eFartOrRoll] = { 1u,{ SDL_SCANCODE_X } };
    mKeyBoardConfig[InputCommands::eGameSpeak1] = { 1u,{ SDL_SCANCODE_1 } };
    mKeyBoardConfig[InputCommands::eGameSpeak2] = { 1u,{ SDL_SCANCODE_2 } };
    mKeyBoardConfig[InputCommands::eGameSpeak3] = { 1u,{ SDL_SCANCODE_3 } };
    mKeyBoardConfig[InputCommands::eGameSpeak4] = { 1u,{ SDL_SCANCODE_4 } };
    mKeyBoardConfig[InputCommands::eGameSpeak5] = { 1u,{ SDL_SCANCODE_5 } };
    mKeyBoardConfig[InputCommands::eGameSpeak6] = { 1u,{ SDL_SCANCODE_6 } };
    mKeyBoardConfig[InputCommands::eGameSpeak7] = { 1u,{ SDL_SCANCODE_7 } };
    mKeyBoardConfig[InputCommands::eGameSpeak8] = { 1u,{ SDL_SCANCODE_8 } };
    mKeyBoardConfig[InputCommands::ePause] = { 1u,{ SDL_SCANCODE_ESCAPE } };
    mKeyBoardConfig[InputCommands::eUnPause] = { 1u,{ SDL_SCANCODE_ESCAPE } };
    mKeyBoardConfig[InputCommands::eCheatMode] = { 1u,{ SDL_SCANCODE_TAB } };

    // Game pad
    mGamePadConfig[InputCommands::eLeft] = { 1u,{ SDL_CONTROLLER_BUTTON_DPAD_LEFT } };
    mGamePadConfig[InputCommands::eRight] = { 1u,{ SDL_CONTROLLER_BUTTON_DPAD_RIGHT } };
    mGamePadConfig[InputCommands::eUp] = { 1u,{ SDL_CONTROLLER_BUTTON_DPAD_UP } };
    mGamePadConfig[InputCommands::eDown] = { 1u,{ SDL_CONTROLLER_BUTTON_DPAD_DOWN } };

    // TODO: Should actually be the trigger axes!
    mGamePadConfig[InputCommands::eChant] = { 2u, { // Only need to match 2 inputs here
        SDL_CONTROLLER_BUTTON_LEFTSHOULDER,
        SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,
        SDL_CONTROLLER_BUTTON_LEFTSTICK,
        SDL_CONTROLLER_BUTTON_RIGHTSTICK } };

    mGamePadConfig[InputCommands::eRun] = { 1u,{ SDL_CONTROLLER_BUTTON_RIGHTSHOULDER } };

    // TODO: Should actually be the trigger axes!
    mGamePadConfig[InputCommands::eSneak] = { 1u,{ SDL_CONTROLLER_BUTTON_RIGHTSTICK } };

    mGamePadConfig[InputCommands::eHop] = { 1u,{ SDL_CONTROLLER_BUTTON_Y } };
    mGamePadConfig[InputCommands::eThrowItem] = { 1u,{ SDL_CONTROLLER_BUTTON_B } };
    mGamePadConfig[InputCommands::eDoAction] = { 1u,{ SDL_CONTROLLER_BUTTON_X } };
    mGamePadConfig[InputCommands::eFartOrRoll] = { 1u,{ SDL_CONTROLLER_BUTTON_A } };
    mGamePadConfig[InputCommands::eGameSpeak1] = { 2u,{ SDL_CONTROLLER_BUTTON_LEFTSHOULDER,  SDL_CONTROLLER_BUTTON_Y } };
    mGamePadConfig[InputCommands::eGameSpeak2] = { 2u,{ SDL_CONTROLLER_BUTTON_LEFTSHOULDER,  SDL_CONTROLLER_BUTTON_B } };
    mGamePadConfig[InputCommands::eGameSpeak3] = { 2u,{ SDL_CONTROLLER_BUTTON_LEFTSHOULDER,  SDL_CONTROLLER_BUTTON_A } };
    mGamePadConfig[InputCommands::eGameSpeak4] = { 2u,{ SDL_CONTROLLER_BUTTON_LEFTSHOULDER,  SDL_CONTROLLER_BUTTON_X } };
    mGamePadConfig[InputCommands::eGameSpeak5] = { 2u,{ SDL_CONTROLLER_BUTTON_RIGHTSHOULDER ,SDL_CONTROLLER_BUTTON_Y } };
    mGamePadConfig[InputCommands::eGameSpeak6] = { 2u,{ SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, SDL_CONTROLLER_BUTTON_B } };
    mGamePadConfig[InputCommands::eGameSpeak7] = { 2u,{ SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, SDL_CONTROLLER_BUTTON_A } };
    mGamePadConfig[InputCommands::eGameSpeak8] = { 2u,{ SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, SDL_CONTROLLER_BUTTON_X } };
    mGamePadConfig[InputCommands::ePause] = { 1u,{ SDL_CONTROLLER_BUTTON_START } };
    mGamePadConfig[InputCommands::eUnPause] = { 1u,{ SDL_CONTROLLER_BUTTON_BACK } };
}

template<class T, class FnIsKeyPressed>
static void DoPressedKeysMatchCommand(FnIsKeyPressed isKeyPressed, u32& pressedCommands, T& command)
{
    // Check how many keys are down that match the required buttons for the given command
    u32 numDowns = 0;
    for (const auto& key : command.second.mButtonsRequired)
    {
        if (isKeyPressed(key))
        {
            numDowns++;
        }
    }

    // If the min number of matching keys are pressed
    if (numDowns >= command.second.mNumberOfKeysRequired)
    {
        // Then the command is down
        pressedCommands |= command.first;
    }
}

u32 InputMapping::GetNewPressedState(const InputReader& input)
{
    u32 pressedCommands = 0;
    for (const auto& keyConfig : mKeyBoardConfig)
    {
        DoPressedKeysMatchCommand([&](SDL_Scancode key)
        { 
            return input.KeyboardKey(key).PressedOrHeld();
        }, pressedCommands, keyConfig);
    }

    if (input.ActiveController())
    {
        for (const auto& keyConfig : mGamePadConfig)
        {
            DoPressedKeysMatchCommand([&](SDL_GameControllerButton button)
            { 
                return input.ActiveController()->mGamePadButtonsPressed[button];
            }, pressedCommands, keyConfig);
        }
    }
    return pressedCommands;
}

void InputMapping::Update(const InputReader& input)
{
    mOldPressedState = mCurrentPressedState;
    mCurrentPressedState = GetNewPressedState(input);

/*
    LOG_INFO(
        "Pressed: "
        << Pressed(InputCommands::eLeft)
        << " Held: "
        << Held(InputCommands::eLeft)
        << " Released: "
        << Released(InputCommands::eLeft));
*/
}

Controller::Controller(SDL_GameController* controller, SDL_Joystick* joyStick) : mController(controller)
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

Controller::~Controller()
{
    TRACE_ENTRYEXIT;
    HapticClose();
    SDL_GameControllerClose(mController);
}

void Controller::OnControllerButton(const SDL_ControllerButtonEvent& event)
{
    mGamePadButtonsPressed[event.button] = event.type == SDL_CONTROLLERBUTTONDOWN;
}

void Controller::OnControllerAxis(const SDL_ControllerAxisEvent& event)
{
    mGamePadAxes[event.axis] = event.value;
}

void Controller::Rumble(f32 strength)
{
    if (mHaptic)
    {
        if (SDL_HapticRumblePlay(mHaptic, strength, 300) != 0)
        {
            LOG_ERROR("SDL_HapticRumblePlay: " << SDL_GetError());
        }
    }
}

void Controller::HapticClose()
{
    if (mHaptic)
    {
        SDL_HapticClose(mHaptic);
        mHaptic = nullptr;
    }
}

void InputReader::Update()
{
    mMapping.Update(*this);

    for (ButtonState& button : mKeyboardKeys)
    {
        button.Update();
    }

//    mKeyboardKeys[SDL_SCANCODE_W].Debug();
}

InputReader::~InputReader()
{
    for (auto& c : mControllers)
    {
        c.second.release();
    }
}

void InputReader::OnKeyEvent(const SDL_KeyboardEvent& event)
{
    // Update state from polling loop
    const u32 keyCode = event.keysym.scancode;
    mKeyboardKeys[keyCode].OnPressed(event.type == SDL_KEYDOWN);
}

void InputReader::OnMouseButtonEvent(const SDL_MouseButtonEvent& event)
{
    // Update state from polling loop
    if (event.button == SDL_BUTTON_LEFT)
    {
        mMouseButtons[0].OnPressed(event.type == SDL_MOUSEBUTTONDOWN);
    }
    else if (event.button == SDL_BUTTON_RIGHT)
    {
        mMouseButtons[1].OnPressed(event.type == SDL_MOUSEBUTTONDOWN);
    }
}

void InputReader::AddControllers()
{
    // Add all of the controllers that are currently plugged in
    for (s32 i = 0; i < SDL_NumJoysticks(); i++)
    {
        LOG_INFO("Adding controller (static)");
        AddController(i);
    }
}

void InputReader::AddController(const SDL_ControllerDeviceEvent& event)
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

void InputReader::RemoveController(const SDL_ControllerDeviceEvent& event)
{
    LOG_INFO("Removing controller");
    auto it = mControllers.find(event.which);
    if (it != std::end(mControllers))
    {
        mControllers.erase(it);
    }
}

void InputReader::OnControllerButton(const SDL_ControllerButtonEvent& event)
{
    auto it = mControllers.find(event.which);
    if (it != std::end(mControllers))
    {
        it->second->OnControllerButton(event);
    }
}

void InputReader::OnControllerAxis(const SDL_ControllerAxisEvent& event)
{
    auto it = mControllers.find(event.which);
    if (it != std::end(mControllers))
    {
        it->second->OnControllerAxis(event);
    }
}

const Controller* InputReader::ActiveController() const
{
    if (mControllers.empty())
    {
        return nullptr;
    }
    return mControllers.begin()->second.get();
}

void InputReader::AddController(s32 index)
{
    if (SDL_IsGameController(index))
    {
        SDL_GameController* controller = SDL_GameControllerOpen(index);
        if (controller)
        {
            SDL_Joystick* joyStick = SDL_GameControllerGetJoystick(controller);
            const SDL_JoystickID id = SDL_JoystickInstanceID(joyStick);
            mControllers[id] = std::make_unique<Controller>(controller, joyStick);
        }
        else
        {
            LOG_ERROR("Failed to open game controller " << index << " due to error: " << SDL_GetError());
        }
    }
}

void InputReader::SetMousePos(s32 x, s32 y)
{
    mMousePosition.mX = x;
    mMousePosition.mY = y;
}
