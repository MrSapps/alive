#pragma once

#include "input.hpp"
#include "core/component.hpp"

class PhysicsComponent;
class AnimationComponent;

class SligMovementComponent : public Component
{
public:
    DECLARE_COMPONENT(SligMovementComponent)
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
    void Update();
private:
    const Actions* mInputMappingActions = nullptr;
    SligMovementComponent* mSligMovement = nullptr;
};