#pragma once

#include <map>
#include <string>
#include <functional>

#include "input.hpp"
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
    void Serialize(std::ostream& os) const final;
    void Deserialize(std::istream& is) final;

public:
    enum Direction : s8
    {
        eLeft = -1,
        eRight = 1
    };
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
        eStandingToRunning,
        eStandingToCrouching,
        eStandingTurningAround,

        eWalking,
        eWalkingToRunning,
        eWalkingToStanding,

        eRunning,
        eRunningToWalking,
        eRunningToStanding,
        eRunningTurningAround,
        eRunningTurningAroundToWalking,
        eRunningTurningAroundToRunning,

        eChanting,
        eChantToStanding,

        eCrouching,
        eCrouchingToRolling,
        eCrouchingToStanding,
        eCrouchingTurningAround,

        eRolling,
        eRollingToWalkingOrRunning,
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
    void StandingToWalking();
    void StandingToRunning();
    void StandingToCrouching();
    void StandingTurningAround();

    void PreChanting(States previous);
    void Chanting();

    void PreWalking(States previous);
    void Walking();
    void WalkingToStanding();

    void PreRunning(States previous);
    void Running();
    void RunningToStanding();
    void RunningTurningAround();
    void RunningTurningAroundToWalking();
    void RunningTurningAroundToRunning();

    void PreCrouching(States previous);
    void Crouching();
    void CrouchingToStanding();
    void CrouchingTurningAround();

    void PreRolling(States previous);
    void Rolling();
    void RollingToWalkingOrRunning();

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
    void FlipDirection();

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
        s8 mDirection;
        bool mRunning;
        bool mSneaking;
        Goal mGoal;
        States mState;
        States mNextState;
    } mData =
        {
            Direction::eRight,
            false,
            false,
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