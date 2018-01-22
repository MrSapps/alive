#pragma once

#include <map>
#include <functional>

#include "input.hpp"
#include "core/component.hpp"

class PhysicsComponent;
class AnimationComponentWithMeta;

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
    AnimationComponentWithMeta* mAnimationComponent = nullptr;

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
    
    struct AnimationData
    {
        const char* mName;
        const std::vector<u32>* mSnapXFrames;
    };

    struct TransistionData
    {
        const AnimationData* mAnimation;
        const AnimationData* mNextAnimation;
        f32 mXSpeed;
        bool mFlipDirection;
        States mNextState;
    };

    const std::vector<u32> kAbeStandIdleSnapXFrames =       { 5 + 1, 14 + 1 };

    const AnimationData kAbeStandTurnAroundAnim =           { "AbeStandTurnAround",           nullptr };
    const AnimationData kAbeStandIdleAnim =                 { "AbeStandIdle",                 nullptr };
    const AnimationData kAbeStandToWalkAnim =               { "AbeStandToWalk",               nullptr };
    const AnimationData kAbeWalkingAnim =                   { "AbeWalking",                   &kAbeStandIdleSnapXFrames };
    const AnimationData kAbeChantToStandAnim =              { "AbeChantToStand",              nullptr };

    const TransistionData kTurnAround =     { &kAbeStandTurnAroundAnim, &kAbeStandIdleAnim,     0.0f,       true,       States::eStanding };
    const TransistionData kStandToWalk =    { &kAbeStandToWalkAnim,     &kAbeWalkingAnim,       kWalkSpeed, false,      States::eWalking  };
    const TransistionData kChantToStand =   { &kAbeChantToStandAnim,    &kAbeStandIdleAnim,     0.0f,       false,      States::eStanding };

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