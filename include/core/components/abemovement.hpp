#pragma once

#include <map>
#include <functional>

#include "input.hpp"
#include "core/component.hpp"

class PhysicsComponent;
class AnimationComponent;

const f32 kWalkSpeed = 2.777771f;

class AbeMovementComponent final : public Component
{
public:
    DECLARE_COMPONENT(AbeMovementComponent);
public:
    void Load();
    void Update();
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
        eChanting,
        eWalkingToStanding,
        eWalking,
    };

    std::map<States, std::function<void()>> mStateFnMap;
    States mState = States::eStanding;

    void SetXSpeed(f32 speed);
    
    struct TransistionData
    {
        const char* mAnimation;
        const char* mNextAnimation;
        f32 mXSpeed;
        bool mFlipDirection;
        States mNextState;
    };

    const TransistionData kTurnAround = { "AbeStandTurnAround", "AbeStandIdle", 0.0f, true, States::eStanding };
    const TransistionData kStandToWalk = { "AbeStandToWalk", "AbeWalking", kWalkSpeed, false, States::eWalking };
    const TransistionData kChantToStand = { "AbeChantToStand", "AbeStandIdle", 0.0f, false, States::eStanding };

    void SetTransistionData(const TransistionData* data);

    const TransistionData* mNextTransistionData = nullptr;
};

class AbePlayerControllerComponent final : public Component
{
public:
    void Load(const InputState& state); // TODO: Input is wired here
    void Update();
private:
    const Actions* mInputMappingActions = nullptr;
    AbeMovementComponent* mAbeMovement = nullptr;
};