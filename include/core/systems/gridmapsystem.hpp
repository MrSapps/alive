#pragma once

#include "core/system.hpp"

class GridmapSystem final : public System
{
public:
    DECLARE_SYSTEM(GridmapSystem);
public:
    void MoveToCamera(const char* cameraName);
    void Update() final;
};
