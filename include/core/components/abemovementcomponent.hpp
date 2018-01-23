#pragma once

#include <map>
#include <functional>

#include "input.hpp"
#include "core/component.hpp"

class PhysicsComponent;
class AnimationComponent;
class TransformComponent;

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
    TransformComponent* mTransformComponent = nullptr;
    AnimationComponent* mAnimationComponent = nullptr;

    enum class States
    {
        eStanding,
        eStandToWalking,
        eStandTurningAround,
        eWalkingToStanding,
        eWalking,
        eChanting,
        eChantToStand,
    };

    struct StateData
    {
        std::function<void(AbeMovementComponent*, States)> mPreHandler;
        std::function<void(AbeMovementComponent*)> mHandler;
    };

    void SetXSpeed(f32 speed);
    
    struct AnimationData
    {
        const char* mName;
    };

    const AnimationData kAbeStandTurnAroundAnim =           { "AbeStandTurnAround" };
    const AnimationData kAbeStandIdleAnim =                 { "AbeStandIdle" };
    const AnimationData kAbeStandToWalkAnim =               { "AbeStandToWalk"};
    const AnimationData kAbeWalkingAnim =                   { "AbeWalking" };
    const AnimationData kAbeChantToStandAnim =              { "AbeChantToStand" };
    const AnimationData kAbeStandToChantAnim =              { "AbeStandToChant" };
    const AnimationData kAbeWalkToStandAnim1 =              { "AbeWalkToStand" };
    const AnimationData kAbeWalkToStandAnim2 =              { "AbeWalkToStandMidGrid" };

    std::map<States, StateData> mStateFnMap;
    States mState = States::eStanding;
    States mNextState = States::eStanding;

    void PreStanding(States previous);
    void Standing();

    void PreChanting(States previous);
    void Chanting();

    void PreWalking(States previous);
    void Walking();

    void StandTurnAround();

    void ASyncTransistion();

    bool DirectionChanged() const;
    bool TryMoveLeftOrRight() const;

    void SetAnimation(const AnimationData& anim);
    void SetState(States state);
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