#pragma once

#include "input.hpp"
#include "core/component.hpp"
#include <map>
#include <functional>

class PhysicsComponent;
class AnimationComponent;

class AbeMovementComponent final : public Component
{
public:
    void Load();
    void Update() override;
public:
    enum class Goal
    {
        eStand,
        eGoLeft,
        eGoRight,
        eChant
    };
    Goal mGoal = Goal::eStand;
private:
    const Actions* mInputMappingActions = nullptr;
    PhysicsComponent* mPhysicsComponent = nullptr;
    AnimationComponent* mAnimationComponent = nullptr;

    enum class States
    {
        eStanding,
        eStandingTurnAround,
        eChanting,
        eChantToStand,
        eStandingToWalking,
        eWalkingToStanding,
        eWalking,
    };

    std::map<States, std::function<void()>> mStateFnMap;
    States mState = States::eStanding;
};

class AbePlayerControllerComponent final : public Component
{
public:
    void Load(const InputState& state); // TODO: Input is wired here
    void Update() override;
private:
    const Actions* mInputMappingActions = nullptr;
    AbeMovementComponent* mAbeMovement = nullptr;
};