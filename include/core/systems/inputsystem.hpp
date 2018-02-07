#pragma once

#include "input.hpp"
#include "core/system.hpp"

class InputSystem final : public System
{
public:
    DECLARE_SYSTEM(InputSystem);

public:
    explicit InputSystem(InputReader& state);

public:
    void Update() final;
    const InputMapping& Mapping() const;
private:
    InputReader& mInput;
};
