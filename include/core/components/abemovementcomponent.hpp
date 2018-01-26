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
    friend class AbePlayerControllerComponent;

public:
    DECLARE_COMPONENT(AbeMovementComponent);

public:
    void OnLoad() final;
    void OnResolveDependencies() final;

public:
    void Serialize(std::ostream &os) const final;
    void Deserialize(std::istream &is) final;

public:
    enum class Goal
    {
        eStand,
        eGoLeft,
        eGoRight,
        eChant
    };

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

public:
    void Update();

private:
    void ASyncTransition();

    void SetXSpeed(f32 speed);
    
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

private:
    std::map<States, StateData> mStateFnMap;
    PhysicsComponent* mPhysicsComponent = nullptr;
    TransformComponent* mTransformComponent = nullptr;
    AnimationComponent* mAnimationComponent = nullptr;

private:
    struct {
        Goal mGoal;
        States mState;
        States mNextState;
    } mData;
};

class AbePlayerControllerComponent final : public Component
{
public:
    DECLARE_COMPONENT(AbePlayerControllerComponent);

public:
    void OnResolveDependencies() final;

public:
    void Update();

private:
    const Actions* mInputMappingActions = nullptr;
    AbeMovementComponent* mAbeMovement = nullptr;
};