#pragma once

#include "input.hpp"
#include "core/system.hpp"

class InputSystem final : public System
{
public:
    DECLARE_SYSTEM(InputSystem);

public:
    explicit InputSystem(const InputState& state);

public:
    void Update() final;

public:
    const Actions* GetActions() const;

private:
    const Actions* mActions = nullptr;
};