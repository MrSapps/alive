#pragma once

#include <map>
#include <functional>

#include "core/component.hpp"

class PhysicsComponent;
class AnimationComponent;
class TransformComponent;

const f32 kAbeWalkSpeed = 2.777771f;

class AbeMovementComponent final : public Component
{
public:
    DECLARE_COMPONENT(AbeMovementComponent);

public:
    void Deserialize(std::istream& is) override;

public:
    void Load() final;
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
        eChantToStand,
    };

    struct StateData
    {
        std::function<void(AbeMovementComponent*, States)> mPreHandler;
        std::function<void(AbeMovementComponent*)> mHandler;
    };

    void ASyncTransition();

    void SetXSpeed(f32 speed);
    
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

    bool DirectionChanged() const;
    bool TryMoveLeftOrRight() const;

    void SetAnimation(const std::string& anim);
    void SetState(States state);
};

class AbePlayerControllerComponent final : public Component
{
public:
    DECLARE_COMPONENT(AbePlayerControllerComponent);

public:
    void Deserialize(std::istream& is) override;

public:
    void Load() final;
    void Update();

private:
    const Actions* mInputMappingActions = nullptr;
    AbeMovementComponent* mAbeMovement = nullptr;
};