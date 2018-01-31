#include "input.hpp"


InputMapping::InputMapping()
{
    // Keyboard
    mKeyBoardConfig[Actions::eLeft] = { 1u,{ SDL_SCANCODE_LEFT } };
    mKeyBoardConfig[Actions::eRight] = { 1u,{ SDL_SCANCODE_RIGHT } };
    mKeyBoardConfig[Actions::eUp] = { 1u,{ SDL_SCANCODE_UP } };
    mKeyBoardConfig[Actions::eDown] = { 1u,{ SDL_SCANCODE_DOWN } };
    mKeyBoardConfig[Actions::eChant] = { 1u,{ SDL_SCANCODE_0 } };
    mKeyBoardConfig[Actions::eRun] = { 1u,{ SDL_SCANCODE_LSHIFT,    SDL_SCANCODE_RSHIFT } };
    mKeyBoardConfig[Actions::eSneak] = { 1u,{ SDL_SCANCODE_LALT,      SDL_SCANCODE_RALT } };
    mKeyBoardConfig[Actions::eJump] = { 1u,{ SDL_SCANCODE_SPACE } };
    mKeyBoardConfig[Actions::eThrow] = { 1u,{ SDL_SCANCODE_Z } };
    mKeyBoardConfig[Actions::eAction] = { 1u,{ SDL_SCANCODE_LCTRL,     SDL_SCANCODE_RCTRL } };
    mKeyBoardConfig[Actions::eRollOrFart] = { 1u,{ SDL_SCANCODE_X } };
    mKeyBoardConfig[Actions::eGameSpeak1] = { 1u,{ SDL_SCANCODE_1 } };
    mKeyBoardConfig[Actions::eGameSpeak2] = { 1u,{ SDL_SCANCODE_2 } };
    mKeyBoardConfig[Actions::eGameSpeak3] = { 1u,{ SDL_SCANCODE_3 } };
    mKeyBoardConfig[Actions::eGameSpeak4] = { 1u,{ SDL_SCANCODE_4 } };
    mKeyBoardConfig[Actions::eGameSpeak5] = { 1u,{ SDL_SCANCODE_5 } };
    mKeyBoardConfig[Actions::eGameSpeak6] = { 1u,{ SDL_SCANCODE_6 } };
    mKeyBoardConfig[Actions::eGameSpeak7] = { 1u,{ SDL_SCANCODE_7 } };
    mKeyBoardConfig[Actions::eGameSpeak8] = { 1u,{ SDL_SCANCODE_8 } };
    mKeyBoardConfig[Actions::eBack] = { 1u,{ SDL_SCANCODE_ESCAPE } };

    // Game pad
    mGamePadConfig[Actions::eLeft] = { 1u,{ SDL_CONTROLLER_BUTTON_DPAD_LEFT } };
    mGamePadConfig[Actions::eRight] = { 1u,{ SDL_CONTROLLER_BUTTON_DPAD_RIGHT } };
    mGamePadConfig[Actions::eUp] = { 1u,{ SDL_CONTROLLER_BUTTON_DPAD_UP } };
    mGamePadConfig[Actions::eDown] = { 1u,{ SDL_CONTROLLER_BUTTON_DPAD_DOWN } };

    // TODO: Should actually be the trigger axes!
    mGamePadConfig[Actions::eChant] = { 2u,{
        SDL_CONTROLLER_BUTTON_LEFTSHOULDER,
        SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,
        SDL_CONTROLLER_BUTTON_LEFTSTICK,
        SDL_CONTROLLER_BUTTON_RIGHTSTICK } };

    mGamePadConfig[Actions::eRun] = { 1u,{ SDL_CONTROLLER_BUTTON_RIGHTSHOULDER } };

    // TODO: Should actually be the trigger axes!
    mGamePadConfig[Actions::eSneak] = { 1u,{ SDL_CONTROLLER_BUTTON_RIGHTSTICK } };

    mGamePadConfig[Actions::eJump] = { 1u,{ SDL_CONTROLLER_BUTTON_Y } };
    mGamePadConfig[Actions::eThrow] = { 1u,{ SDL_CONTROLLER_BUTTON_B } };
    mGamePadConfig[Actions::eAction] = { 1u,{ SDL_CONTROLLER_BUTTON_X } };
    mGamePadConfig[Actions::eRollOrFart] = { 1u,{ SDL_CONTROLLER_BUTTON_A } };
    mGamePadConfig[Actions::eGameSpeak1] = { 2u,{ SDL_CONTROLLER_BUTTON_LEFTSHOULDER,  SDL_CONTROLLER_BUTTON_Y } };
    mGamePadConfig[Actions::eGameSpeak2] = { 2u,{ SDL_CONTROLLER_BUTTON_LEFTSHOULDER,  SDL_CONTROLLER_BUTTON_B } };
    mGamePadConfig[Actions::eGameSpeak3] = { 2u,{ SDL_CONTROLLER_BUTTON_LEFTSHOULDER,  SDL_CONTROLLER_BUTTON_A } };
    mGamePadConfig[Actions::eGameSpeak4] = { 2u,{ SDL_CONTROLLER_BUTTON_LEFTSHOULDER,  SDL_CONTROLLER_BUTTON_X } };
    mGamePadConfig[Actions::eGameSpeak5] = { 2u,{ SDL_CONTROLLER_BUTTON_RIGHTSHOULDER ,SDL_CONTROLLER_BUTTON_Y } };
    mGamePadConfig[Actions::eGameSpeak6] = { 2u,{ SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, SDL_CONTROLLER_BUTTON_B } };
    mGamePadConfig[Actions::eGameSpeak7] = { 2u,{ SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, SDL_CONTROLLER_BUTTON_A } };
    mGamePadConfig[Actions::eGameSpeak8] = { 2u,{ SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, SDL_CONTROLLER_BUTTON_X } };
    mGamePadConfig[Actions::eBack] = { 1u,{ SDL_CONTROLLER_BUTTON_START } };
}

void InputMapping::Update(const InputState& input)
{
    for (const auto& cfg : mKeyBoardConfig)
    {
        u32 numDowns = 0;
        for (const auto& key : cfg.second.mKeys)
        {
            if (input.mKeys[key].RawDownState())
            {
                numDowns++;
            }
        }
        mActions.SetRawDownState(cfg.first, numDowns >= cfg.second.mNumberOfKeysRequired);
    }

    if (input.ActiveController())
    {
        for (const auto& cfg : mGamePadConfig)
        {
            // The keyboard key isn't pressed so take the controller key state
            if (mActions.RawDownState(cfg.first) == false)
            {
                u32 numDowns = 0;
                for (const auto& key : cfg.second.mButtons)
                {
                    if (input.ActiveController()->mGamePadButtons[key].RawDownState())
                    {
                        numDowns++;
                    }
                }
                mActions.SetRawDownState(cfg.first, numDowns >= cfg.second.mNumberOfKeysRequired);
            }
        }
    }

    mActions.UpdateStates();
}