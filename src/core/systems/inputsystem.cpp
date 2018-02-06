#include "core/systems/inputsystem.hpp"

DEFINE_SYSTEM(InputSystem);

InputSystem::InputSystem(const InputState& state) : mActions(&state.Mapping().GetActions())
{

}

const Actions* InputSystem::GetActions() const
{
    return mActions;
}

void InputSystem::Update()
{

}
