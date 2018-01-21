#pragma once

#include "core/component.hpp"

class SligMovementComponent : public Component
{
public:
    void Load();
public:
    bool mLeft = false;
    bool mRight = false;
private:
    PhysicsComponent* mPhysicsComponent = nullptr;
    AnimationComponent* mAnimationComponent = nullptr;
};

class SligPlayerControllerComponent final : public Component
{
public:
    void Load(const InputState& state); // TODO: Input is wired here
    void Update() override;
private:
    const Actions* mInputMappingActions = nullptr;
    SligMovementComponent* mSligMovement = nullptr;
};