#pragma once

#include "core/system.hpp"

class DebugSystem final : public System
{
public:
    DECLARE_SYSTEM(DebugSystem);

public:
    void Update() final;
};