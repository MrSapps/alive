#pragma once

#include "core/component.hpp"

class TransformComponent;

class PhysicsComponent final : public Component
{
public:
    DECLARE_COMPONENT(PhysicsComponent)
public:
    float xSpeed = 0.0f;
    float ySpeed = 0.0f;
};