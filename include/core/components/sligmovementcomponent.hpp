#pragma once

#include <map>
#include <functional>

#include "core/component.hpp"

class PhysicsComponent;
class AnimationComponent;
class TransformComponent;

const f32 kSligWalkSpeed = 2.777771f;

class SligMovementComponent final : public Component
{
public:
	friend class SligPlayerControllerComponent;

public:
    DECLARE_COMPONENT(SligMovementComponent);

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
    };

    struct StateData
    {
        std::function<void(SligMovementComponent*, States)> mPreHandler;
        std::function<void(SligMovementComponent*)> mHandler;
    };

    struct AnimationData
    {
        const char* mName;
    };

    const AnimationData kSligStandTurnAroundAnim = { "SligStandTurnAround" };
    const AnimationData kSligStandIdleAnim = { "SligStandIdle" };
    const AnimationData kSligStandToWalkAnim = { "SligStandToWalk" };
    const AnimationData kSligWalkingAnim = { "SligWalking2" };
    const AnimationData kSligWalkToStandAnim1 = { "SligWalkToStand" };
    const AnimationData kSligWalkToStandAnim2 = { "SligWalkToStandMidGrid" };

public:
    void Update();

    void SetXSpeed(f32 speed);

    void PreStanding(States previous);
    void Standing();

    void PreWalking(States previous);
    void Walking();

    void StandTurnAround();

    void ASyncTransition();

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
        States mState = States::eStanding;
        States mNextState = States::eStanding;
    } mData;
};

class SligPlayerControllerComponent final : public Component
{
public:
    DECLARE_COMPONENT(SligPlayerControllerComponent);

public:
    void OnResolveDependencies() final;

public:
    void Update();

private:
    const Actions* mInputMappingActions = nullptr;
    SligMovementComponent* mSligMovement = nullptr;
};