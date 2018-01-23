#pragma once

#include <map>
#include <functional>

#include "input.hpp"
#include "core/component.hpp"

class PhysicsComponent;
class AnimationComponent;
class TransformComponent;

const f32 kSligWalkSpeed = 2.777771f;

class SligMovementComponent final : public Component
{
public:
    DECLARE_COMPONENT(SligMovementComponent);
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
    };

    struct StateData
    {
        std::function<void(SligMovementComponent*, States)> mPreHandler;
        std::function<void(SligMovementComponent*)> mHandler;
    };

    void SetXSpeed(f32 speed);

    struct AnimationData
    {
        const char* mName;
    };

    const AnimationData kSligStandTurnAroundAnim =           { "SligStandTurnAround" };
    const AnimationData kSligStandIdleAnim =                 { "SligStandIdle" };
    const AnimationData kSligStandToWalkAnim =               { "SligStandToWalk"};
    const AnimationData kSligWalkingAnim =                   { "SligWalking2" };
    const AnimationData kSligWalkToStandAnim1 =              { "SligWalkToStand" };
    const AnimationData kSligWalkToStandAnim2 =              { "SligWalkToStandMidGrid" };

    std::map<States, StateData> mStateFnMap;
    States mState = States::eStanding;
    States mNextState = States::eStanding;

    void PreStanding(States previous);
    void Standing();

    void PreWalking(States previous);
    void Walking();

    void StandTurnAround();

    void ASyncTransition();

    bool DirectionChanged() const;
    bool TryMoveLeftOrRight() const;

    void SetAnimation(const AnimationData& anim);
    void SetState(States state);
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