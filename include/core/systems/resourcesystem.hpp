#pragma once

#include "core/system.hpp"

class ResourceSystem final : public System
{
public:
    DECLARE_SYSTEM(ResourceSystem);

public:
    void Update() final;
};