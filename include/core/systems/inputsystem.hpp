#pragma once

#include "input.hpp"
#include "core/component.hpp"

class InputSystem : public Component
{
public:
    DECLARE_COMPONENT(InputSystem);

public:
    void Load(const InputState& state);

public:
    const Actions* GetActions() const;

private:
    const Actions* mActions = nullptr;
};