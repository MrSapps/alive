#include "core/systems/inputsystem.hpp"

DEFINE_SYSTEM(InputSystem);

InputSystem::InputSystem(InputReader& state) : mInput(state)
{

}

void InputSystem::Update()
{
    mInput.Update();
}

const InputMapping& InputSystem::Mapping() const
{
    return mInput.GameCommands();
}