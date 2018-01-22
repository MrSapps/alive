#pragma once

#include "core/component.hpp"

class TransformComponent;

class PhysicsComponent final : public Component
{
public:
    DECLARE_COMPONENT(PhysicsComponent)
public:
    void Load();
    void Update();
    void SnapXToGrid();
public:
    float xSpeed = 0.0f;
    float ySpeed = 0.0f;
private:
    TransformComponent* mTransformComponent = nullptr;
};