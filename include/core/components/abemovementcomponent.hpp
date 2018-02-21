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
        eStanding,
        eStandingToWalking,
        eStandingToRunning,
        eStandingToCrouching,
        eStandingPushingWall,
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

        eCrouching,
        eCrouchingToRolling,
        eCrouchingToStanding,
        eCrouchingTurningAround,

        eRolling,
        eRollingToWalkingOrRunning,

        eFallingBack,
        eFallingBackToStanding,
        eFallingBackToStandingAngry,

        eChanting,
        eChantingToStanding,
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
        eAbeFallBackStanding,
        eAbeFallBackToStand,
    };

    struct StateData
    {
        std::function<void(AbeMovementComponent*, States)> mPreHandler;
        std::function<void(AbeMovementComponent*)> mHandler;
    };

public:
    void Update();
    void ToggleCheatMode();

private:

    void PreStanding(States previous);
    void Standing();
    void PreStandingToWalking(States previous);
    void StandingToWalking();
    void PreStandingToRunning(States previous);
    void StandingToRunning();
    void PreStandingToCrouching(States previous);
    void StandingToCrouching();
    void PreStandingPushingWall(States previous);
    void StandingPushingWall();
    void PreStandingTurningAround(States previous);
    void StandingTurningAround();

    void PreWalking(States previous);
    void Walking();
    void PreWalkingToRunning(States previous);
    void WalkingToRunning();
    void PreWalkingToStanding(States previous);
    void WalkingToStanding();

    void PreRunning(States previous);
    void Running();
    void PreRunningToWalking(States previous);
    void RunningToWalking();
    void PreRunningToStanding(States previous);
    void RunningToStanding();
    void PreRunningTurningAround(States previous);
    void RunningTurningAround();
    void PreRunningTurningAroundToWalking(States previous);
    void RunningTurningAroundToWalking();
    void PreRunningTurningAroundToRunning(States previous);
    void RunningTurningAroundToRunning();

    void PreCrouching(States previous);
    void Crouching();
    void PreCrouchingToRolling(States previous);
    void CrouchingToRolling();
    void PreCrouchingToStanding(States previous);
    void CrouchingToStanding();
    void PreCrouchingTurningAround(States previous);
    void CrouchingTurningAround();

    void PreRolling(States previous);
    void Rolling();
    void PreRollingToWalkingOrRunning(States previous);
    void RollingToWalkingOrRunning();

    void PreFallingBack(States previous);
    void FallingBack();
    void PreFallingBackToStanding(States previous);
    void FallingBackToStanding();
    void PreFallingBackToStandingAngry(States previous);
    void FallingBackToStandingAngry();

    void PreChanting(States previous);
    void Chanting();
    void PreChantingToStanding(States previous);
    void ChantingToStanding();

private:
    void PushWallOrCrouch();

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
    void SetAnimation(AbeAnimation abeAnimation, u32 animationFrame = 0);
    void PlaySoundEffect(const char* fxName);

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
        AbeAnimation mAnimation;
        s32 mAnimationFrame;
        bool mCheatEnabled;
    } mData =
        {
            Direction::eRight,
            false,
            false,
            Goal::eStand,
            States::eStanding,
            AbeAnimation::eAbeStandIdle,
            0,
            false
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
    const  InputMapping* mGameCommands = nullptr;
    AbeMovementComponent* mAbeMovement = nullptr;
};