#pragma once

#include "core/component.hpp"
#include "core/system.hpp"

// Each "floor" of a lift.
class LiftPointComponent : public Component
{
public:
    DECLARE_COMPONENT(LiftPointComponent);

};

// If activated will attempt to move the linked lift
class LiftMoverComponent : public Component
{
public:
    DECLARE_COMPONENT(LiftMoverComponent);

};

class LiftSystem : public System
{
public:
    DECLARE_SYSTEM(LiftSystem);

    void Update();
};
