#pragma once

#include "core/component.hpp"

class TransformComponent;

class PhysicsComponent final : public Component
{
public:
    void Load();
    void Update() final;
    void SnapXToGrid();
public:
    float xSpeed = 0.0f;
    float ySpeed = 0.0f;
private:
    TransformComponent* mTransformComponent = nullptr;
};