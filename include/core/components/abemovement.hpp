#pragma once

#include "input.hpp"
#include "core/component.hpp"

class PhysicsComponent;
class AnimationComponent;

class AbeMovementComponent final : public Component
{
public:
    void Load();
    void Update() override;
public:
    bool mLeft = false;
    bool mRight = false;
private:
    const Actions* mInputMappingActions = nullptr;
    PhysicsComponent* mPhysicsComponent = nullptr;
    AnimationComponent* mAnimationComponent = nullptr;
};

class PlayerControllerComponent final : public Component
{
public:
    void Load(const InputState& state);
    void Update() override;
private:
    const Actions* mInputMappingActions = nullptr;
    AbeMovementComponent* mAbeMovement = nullptr;
};