#pragma once

#include <map>
#include <functional>

#include "core/component.hpp"

class PhysicsComponent;
class AnimationComponent;
class TransformComponent;

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

    void PreStanding(States previous);
    void Standing();

    void PreChanting(States previous);
    void Chanting();

    void PreWalking(States previous);
    void Walking();
    void WalkToStand();

    void StandTurnAround();

    // Helpers

    void SnapXToGrid();
    void SetXSpeed(f32 speed);
    bool FrameIs(u32 frame) const;
    void SetFrame(u32 frame);
    void PlaySoundEffect(const char* fxName);

    bool DirectionChanged() const;
    bool TryMoveLeftOrRight() const;

    void SetAnimation(const std::string& anim);
    void SetState(States state);
    void SetCurrentAndNextState(States current, States next);

private:
    std::map<States, StateData> mStateFnMap;
    PhysicsComponent* mPhysicsComponent = nullptr;
    TransformComponent* mTransformComponent = nullptr;
    AnimationComponent* mAnimationComponent = nullptr;

private:
    struct
    {
        Goal mGoal;
        States mState;
        States mNextState;
    }
    mData =
    {
        Goal::eStand,
        States::eStanding,
        States::eStanding
    };
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