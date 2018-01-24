#include "core/systems/inputsystem.hpp"

DEFINE_COMPONENT(InputSystem);

void InputSystem::Load(const InputState& state)
{
    mActions = &state.Mapping().GetActions();
}

const Actions* InputSystem::GetActions() const
{
    return mActions;
}