#pragma once

#include <map>
#include <string>
#include <functional>

#include "core/component.hpp"

class CollisionSystem;
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
        eGoUp,
        eGoDown,
        eGoLeft,
        eGoRight,
        eChant,
    };

    enum class States
    {
        ePushingWall,
        ePushingWallToStanding,

        eStanding,
        eStandingToWalking,
        eStandingToCrouching,
        eStandingTurningAround,

        eWalking,
        eWalkingToStanding,

        eChanting,
        eChantToStanding,

        eCrouching,
        eCrouchingToStanding
    };

    enum class AbeAnimation : u16
    {
        eAbeWalkToStand,
        eAbeWalkToStandMidGrid,
        eAbeWalkingToRunning,
        eAbeWalkingToRunningMidGrid,
        eAbeWalkingToSneaking,
        eAbeWalkingToSneakingMidGrid,
        eAbeStandToRun,
        eAbeRunningToSkidTurn,
        eAbeRunningTurnAround,
        eAbeRunningTurnAroundToWalk,
        eAbeRunningToRoll,
        eAbeRunningToJump,
        eAbeRunningJumpInAir,
        eAbeLandToRunning,
        eAbeLandToWalking,
        eAbeFallingToLand,
        eAbeRunningSkidStop,
        eAbeRunningToWalk,
        eAbeRunningToWalkingMidGrid,
        eAbeStandToSneak,
        eAbeSneakToStand,
        eAbeSneakToStandMidGrid,
        eAbeSneakingToWalking,
        eAbeSneakingToWalkingMidGrid,
        eAbeStandPushWall,
        eAbeHitGroundToStand,
        eAbeStandToWalk,
        eAbeStandToCrouch,
        eAbeCrouchToStand,
        eAbeStandTurnAround,
        eAbeStandTurnAroundToRunning,
        eAbeCrouchTurnAround,
        eAbeCrouchToRoll,
        eAbeStandSpeak1,
        eAbeStandSpeak2,
        eAbeStandSpeak3,
        eAbeStandingSpeak4,
        eAbeStandSpeak5,
        eAbeCrouchSpeak1,
        eAbeCrouchSpeak2,
        eAbeStandIdle,
        eAbeCrouchIdle,
        eAbeStandToHop,
        eAbeHopping,
        eAbeHoppingToStand,
        eAbeHoistDangling,
        eAbeHoistPullSelfUp,
        eAbeStandToJump,
        eAbeJumpUpFalling,
        eAbeWalking,
        eAbeRunning,
        eAbeSneaking,
        eAbeStandToFallingFromTrapDoor,
        eAbeHoistDropDown,
        eAbeRolling,
        eAbeStandToChant,
        eAbeChantToStand,
        eAbeGassed,
    };

    struct StateData
    {
        std::function<void(AbeMovementComponent*, States)> mPreHandler;
        std::function<void(AbeMovementComponent*)> mHandler;
    };

public:
    void Update();

private:
    void PrePushingWall(States previous);
    void PushingWall();

    void PreStanding(States previous);
    void Standing();
    void StandingTurningAround();
    void StandingToCrouching();

    void PreChanting(States previous);
    void Chanting();

    void PreWalking(States previous);
    void Walking();
    void WalkingToStanding();

    void PreCrouching(States previous);
    void Crouching();
    void CrouchingToStanding();

private:
    void PushWallOrCrouch();

private:
    void ASyncTransition();

private:
    bool DirectionChanged() const;
    bool IsMovingLeftOrRight() const;
    bool IsMovingTowardsWall() const;

private:
    bool FrameIs(u32 frame) const;
    void SetFrame(u32 frame);
    void SetXSpeed(f32 speed);
    void SnapXToGrid();

private:
    void SetState(States state);
    void SetAnimation(const std::string& anim);
    void PlaySoundEffect(const char* fxName);
    void SetCurrentAndNextState(States current, States next);

private:
    std::map<States, StateData> mStateFnMap;

private:
    CollisionSystem* mCollisionSystem = nullptr;
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