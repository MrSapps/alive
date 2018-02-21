#include <ostream>
#include <istream>

#include "core/entity.hpp"
#include "core/entitymanager.hpp"
#include "core/systems/inputsystem.hpp"
#include "core/systems/collisionsystem.hpp"
#include "core/systems/soundsystem.hpp"
#include "core/components/physicscomponent.hpp"
#include "core/components/transformcomponent.hpp"
#include "core/components/animationcomponent.hpp"
#include "core/components/abemovementcomponent.hpp"
#include "core/components/sligmovementcomponent.hpp"

DEFINE_COMPONENT(AbeMovementComponent);

static const f32 kAbeRunSpeed = 6.25;
static const f32 kAbeWalkSpeed = 2.777771f;
static const std::map<AbeMovementComponent::AbeAnimation, std::string> kAbeAnimations = { // NOLINT
    { AbeMovementComponent::AbeAnimation::eAbeWalkToStand, std::string{ "AbeWalkToStand" }},
    { AbeMovementComponent::AbeAnimation::eAbeWalkToStandMidGrid, std::string{ "AbeWalkToStandMidGrid" }},
    { AbeMovementComponent::AbeAnimation::eAbeWalkingToRunning, std::string{ "AbeWalkingToRunning" }},
    { AbeMovementComponent::AbeAnimation::eAbeWalkingToRunningMidGrid, std::string{ "AbeWalkingToRunningMidGrid" }},
    { AbeMovementComponent::AbeAnimation::eAbeWalkingToSneaking, std::string{ "AbeWalkingToSneaking" }},
    { AbeMovementComponent::AbeAnimation::eAbeWalkingToSneakingMidGrid, std::string{ "AbeWalkingToSneakingMidGrid" }},
    { AbeMovementComponent::AbeAnimation::eAbeStandToRun, std::string{ "AbeStandToRun" }},
    { AbeMovementComponent::AbeAnimation::eAbeRunningToSkidTurn, std::string{ "AbeRunningToSkidTurn" }},
    { AbeMovementComponent::AbeAnimation::eAbeRunningTurnAround, std::string{ "AbeRunningTurnAround" }},
    { AbeMovementComponent::AbeAnimation::eAbeRunningTurnAroundToWalk, std::string{ "AbeRunningTurnAroundToWalk" }},
    { AbeMovementComponent::AbeAnimation::eAbeRunningToRoll, std::string{ "AbeRunningToRoll" }},
    { AbeMovementComponent::AbeAnimation::eAbeRunningToJump, std::string{ "AbeRunningToJump" }},
    { AbeMovementComponent::AbeAnimation::eAbeRunningJumpInAir, std::string{ "AbeRunningJumpInAir" }},
    { AbeMovementComponent::AbeAnimation::eAbeLandToRunning, std::string{ "AbeLandToRunning" }},
    { AbeMovementComponent::AbeAnimation::eAbeLandToWalking, std::string{ "AbeLandToWalking" }},
    { AbeMovementComponent::AbeAnimation::eAbeFallingToLand, std::string{ "AbeFallingToLand" }},
    { AbeMovementComponent::AbeAnimation::eAbeRunningSkidStop, std::string{ "AbeRunningSkidStop" }},
    { AbeMovementComponent::AbeAnimation::eAbeRunningToWalk, std::string{ "AbeRunningToWalk" }},
    { AbeMovementComponent::AbeAnimation::eAbeRunningToWalkingMidGrid, std::string{ "AbeRunningToWalkingMidGrid" }},
    { AbeMovementComponent::AbeAnimation::eAbeStandToSneak, std::string{ "AbeStandToSneak" }},
    { AbeMovementComponent::AbeAnimation::eAbeSneakToStand, std::string{ "AbeSneakToStand" }},
    { AbeMovementComponent::AbeAnimation::eAbeSneakToStandMidGrid, std::string{ "AbeSneakToStandMidGrid" }},
    { AbeMovementComponent::AbeAnimation::eAbeSneakingToWalking, std::string{ "AbeSneakingToWalking" }},
    { AbeMovementComponent::AbeAnimation::eAbeSneakingToWalkingMidGrid, std::string{ "AbeSneakingToWalkingMidGrid" }},
    { AbeMovementComponent::AbeAnimation::eAbeStandPushWall, std::string{ "AbeStandPushWall" }},
    { AbeMovementComponent::AbeAnimation::eAbeHitGroundToStand, std::string{ "AbeHitGroundToStand" }},
    { AbeMovementComponent::AbeAnimation::eAbeStandToWalk, std::string{ "AbeStandToWalk" }},
    { AbeMovementComponent::AbeAnimation::eAbeStandToCrouch, std::string{ "AbeStandToCrouch" }},
    { AbeMovementComponent::AbeAnimation::eAbeCrouchToStand, std::string{ "AbeCrouchToStand" }},
    { AbeMovementComponent::AbeAnimation::eAbeStandTurnAround, std::string{ "AbeStandTurnAround" }},
    { AbeMovementComponent::AbeAnimation::eAbeStandTurnAroundToRunning, std::string{ "AbeStandTurnAroundToRunning" }},
    { AbeMovementComponent::AbeAnimation::eAbeCrouchTurnAround, std::string{ "AbeCrouchTurnAround" }},
    { AbeMovementComponent::AbeAnimation::eAbeCrouchToRoll, std::string{ "AbeCrouchToRoll" }},
    { AbeMovementComponent::AbeAnimation::eAbeStandSpeak1, std::string{ "AbeStandSpeak1" }},
    { AbeMovementComponent::AbeAnimation::eAbeStandSpeak2, std::string{ "AbeStandSpeak2" }},
    { AbeMovementComponent::AbeAnimation::eAbeStandSpeak3, std::string{ "AbeStandSpeak3" }},
    { AbeMovementComponent::AbeAnimation::eAbeStandingSpeak4, std::string{ "AbeStandingSpeak4" }},
    { AbeMovementComponent::AbeAnimation::eAbeStandSpeak5, std::string{ "AbeStandSpeak5" }},
    { AbeMovementComponent::AbeAnimation::eAbeCrouchSpeak1, std::string{ "AbeCrouchSpeak1" }},
    { AbeMovementComponent::AbeAnimation::eAbeCrouchSpeak2, std::string{ "AbeCrouchSpeak2" }},
    { AbeMovementComponent::AbeAnimation::eAbeStandIdle, std::string{ "AbeStandIdle" }},
    { AbeMovementComponent::AbeAnimation::eAbeCrouchIdle, std::string{ "AbeCrouchIdle" }},
    { AbeMovementComponent::AbeAnimation::eAbeStandToHop, std::string{ "AbeStandToHop" }},
    { AbeMovementComponent::AbeAnimation::eAbeHopping, std::string{ "AbeHopping" }},
    { AbeMovementComponent::AbeAnimation::eAbeHoppingToStand, std::string{ "AbeHoppingToStand" }},
    { AbeMovementComponent::AbeAnimation::eAbeHoistDangling, std::string{ "AbeHoistDangling" }},
    { AbeMovementComponent::AbeAnimation::eAbeHoistPullSelfUp, std::string{ "AbeHoistPullSelfUp" }},
    { AbeMovementComponent::AbeAnimation::eAbeStandToJump, std::string{ "AbeStandToJump" }},
    { AbeMovementComponent::AbeAnimation::eAbeJumpUpFalling, std::string{ "AbeJumpUpFalling" }},
    { AbeMovementComponent::AbeAnimation::eAbeWalking, std::string{ "AbeWalking" }},
    { AbeMovementComponent::AbeAnimation::eAbeRunning, std::string{ "AbeRunning" }},
    { AbeMovementComponent::AbeAnimation::eAbeSneaking, std::string{ "AbeSneaking" }},
    { AbeMovementComponent::AbeAnimation::eAbeStandToFallingFromTrapDoor, std::string{ "AbeStandToFallingFromTrapDoor" }},
    { AbeMovementComponent::AbeAnimation::eAbeHoistDropDown, std::string{ "AbeHoistDropDown" }},
    { AbeMovementComponent::AbeAnimation::eAbeRolling, std::string{ "AbeRolling" }},
    { AbeMovementComponent::AbeAnimation::eAbeStandToChant, std::string{ "AbeStandToChant" }},
    { AbeMovementComponent::AbeAnimation::eAbeChantToStand, std::string{ "AbeChantToStand" }},
    { AbeMovementComponent::AbeAnimation::eAbeGassed, std::string{ "AbeGassed" }},
    { AbeMovementComponent::AbeAnimation::eAbeFallBackStanding, std::string{ "AbeFallBackStanding" }},
    { AbeMovementComponent::AbeAnimation::eAbeFallBackToStand, std::string{ "AbeFallBackToStand" }},
};

void AbeMovementComponent::OnLoad()
{

    mStateFnMap[States::eStanding] = { &AbeMovementComponent::PreStanding, &AbeMovementComponent::Standing };
    mStateFnMap[States::eStandingToWalking] = { &AbeMovementComponent::PreStandingToWalking, &AbeMovementComponent::StandingToWalking };
    mStateFnMap[States::eStandingToRunning] = { &AbeMovementComponent::PreStandingToRunning, &AbeMovementComponent::StandingToRunning };
    mStateFnMap[States::eStandingToCrouching] = { &AbeMovementComponent::PreStandingToCrouching, &AbeMovementComponent::StandingToCrouching };
    mStateFnMap[States::eStandingPushingWall] = { &AbeMovementComponent::PreStandingPushingWall, &AbeMovementComponent::StandingPushingWall };
    mStateFnMap[States::eStandingTurningAround] = { &AbeMovementComponent::PreStandingTurningAround, &AbeMovementComponent::StandingTurningAround };

    mStateFnMap[States::eRunning] = { &AbeMovementComponent::PreRunning, &AbeMovementComponent::Running };
    mStateFnMap[States::eRunningToWalking] = { &AbeMovementComponent::PreRunningToWalking, &AbeMovementComponent::RunningToWalking };
    mStateFnMap[States::eRunningToStanding] = { &AbeMovementComponent::PreRunningToStanding, &AbeMovementComponent::RunningToStanding };
    mStateFnMap[States::eRunningTurningAround] = { &AbeMovementComponent::PreRunningTurningAround, &AbeMovementComponent::RunningTurningAround };
    mStateFnMap[States::eRunningTurningAroundToWalking] = { &AbeMovementComponent::PreRunningTurningAroundToWalking, &AbeMovementComponent::RunningTurningAroundToWalking };
    mStateFnMap[States::eRunningTurningAroundToRunning] = { &AbeMovementComponent::PreRunningTurningAroundToRunning, &AbeMovementComponent::RunningTurningAroundToRunning };

    mStateFnMap[States::eWalking] = { &AbeMovementComponent::PreWalking, &AbeMovementComponent::Walking };
    mStateFnMap[States::eWalkingToRunning] = { &AbeMovementComponent::PreWalkingToRunning, &AbeMovementComponent::WalkingToRunning };
    mStateFnMap[States::eWalkingToStanding] = { &AbeMovementComponent::PreWalkingToStanding, &AbeMovementComponent::WalkingToStanding };

    mStateFnMap[States::eChanting] = { &AbeMovementComponent::PreChanting, &AbeMovementComponent::Chanting };
    mStateFnMap[States::eChantingToStanding] = { &AbeMovementComponent::PreChantingToStanding, &AbeMovementComponent::ChantingToStanding };

    mStateFnMap[States::eCrouching] = { &AbeMovementComponent::PreCrouching, &AbeMovementComponent::Crouching };
    mStateFnMap[States::eCrouchingToRolling] = { &AbeMovementComponent::PreCrouchingToRolling, &AbeMovementComponent::CrouchingToRolling };
    mStateFnMap[States::eCrouchingToStanding] = { &AbeMovementComponent::PreCrouchingToStanding, &AbeMovementComponent::CrouchingToStanding };
    mStateFnMap[States::eCrouchingTurningAround] = { &AbeMovementComponent::PreCrouchingTurningAround, &AbeMovementComponent::CrouchingTurningAround };

    mStateFnMap[States::eRolling] = { &AbeMovementComponent::PreRolling, &AbeMovementComponent::Rolling };
    mStateFnMap[States::eRollingToWalkingOrRunning] = { &AbeMovementComponent::PreRollingToWalkingOrRunning, &AbeMovementComponent::RollingToWalkingOrRunning };

    mStateFnMap[States::eFallingBack] = { &AbeMovementComponent::PreFallingBack, &AbeMovementComponent::FallingBack };
    mStateFnMap[States::eFallingBackToStanding] = { &AbeMovementComponent::PreFallingBackToStanding, &AbeMovementComponent::FallingBackToStanding };
    mStateFnMap[States::eFallingBackToStandingAngry] = { &AbeMovementComponent::PreFallingBackToStandingAngry, &AbeMovementComponent::FallingBackToStandingAngry };

    Component::OnLoad(); // calls OnResolveDependencies
}

void AbeMovementComponent::OnResolveDependencies()
{
    mCollisionSystem = mEntity.GetManager()->GetSystem<CollisionSystem>();

    mPhysicsComponent = mEntity.GetComponent<PhysicsComponent>();
    mAnimationComponent = mEntity.GetComponent<AnimationComponent>();
    mTransformComponent = mEntity.GetComponent<TransformComponent>();

    for (auto const& animationName : kAbeAnimations)
    {
        mAnimationComponent->Load(animationName.second);
    }

    SetAnimation(mData.mAnimation, static_cast<u32>(mData.mAnimationFrame));
    mAnimationComponent->mFlipX = mData.mDirection == Direction::eLeft;
}

void AbeMovementComponent::Serialize(std::ostream& os) const
{
    static_assert(std::is_pod<decltype(mData)>::value, "AbeMovementComponent::mData is not a POD type");
    os.write(static_cast<const char*>(static_cast<const void*>(&mData)), sizeof(decltype(mData)));
}

void AbeMovementComponent::Deserialize(std::istream& is)
{
    static_assert(std::is_pod<decltype(mData)>::value, "AbeMovementComponent::mData is not a POD type");
    is.read(static_cast<char*>(static_cast<void*>(&mData)), sizeof(decltype(mData)));
}

void AbeMovementComponent::Update()
{
    if (mData.mCheatEnabled)
    {
        if (mData.mGoal == Goal::eGoDown)
        {
            mTransformComponent->AddY(mData.mRunning ? kAbeRunSpeed : kAbeWalkSpeed);
        }
        else if (mData.mGoal == Goal::eGoUp)
        {
            mTransformComponent->AddY(mData.mRunning ? -kAbeRunSpeed : -kAbeWalkSpeed);
        }
        else if (mData.mGoal == Goal::eGoLeft)
        {
            mTransformComponent->AddX(mData.mRunning ? -kAbeRunSpeed : -kAbeWalkSpeed);
        }
        else if (mData.mGoal == Goal::eGoRight)
        {
            mTransformComponent->AddX(mData.mRunning ? kAbeRunSpeed : kAbeWalkSpeed);
        }
    }
    else
    {
        mStateFnMap.find(mData.mState)->second.mHandler(this);
    }
    mData.mAnimationFrame = mAnimationComponent->FrameNumber();
}

/**
 * Abe Movement Finite State Machine
 */

void AbeMovementComponent::PreStanding(AbeMovementComponent::States)
{
    SetAnimation(AbeAnimation::eAbeStandIdle);
    mPhysicsComponent->SetXSpeed(0.0f);
    mPhysicsComponent->SetYSpeed(0.0f);
}

void AbeMovementComponent::Standing()
{
    if (IsMovingLeftOrRight())
    {
        if (DirectionChanged())
        {
            SetState(States::eStandingTurningAround);
        }
        else
        {
            if (IsMovingTowardsWall())
            {
                GoToStandingPushingWallOrCrouching();
            }
            else
            {
                if (mData.mRunning)
                {
                    SetState(States::eStandingToRunning);
                }
                else
                {
                    SetState(States::eStandingToWalking);
                }
            }
        }
    }
    else if (mData.mGoal == Goal::eGoDown)
    {
        SetState(States::eStandingToCrouching);
    }
    else if (mData.mGoal == Goal::eChant)
    {
        SetState(States::eChanting);
    }
}

void AbeMovementComponent::PreStandingToWalking(AbeMovementComponent::States)
{
    SetXSpeed(kAbeWalkSpeed);
    SetAnimation(AbeAnimation::eAbeStandToWalk);
}

void AbeMovementComponent::StandingToWalking()
{
    if (mAnimationComponent->Complete())
    {
        SetState(States::eWalking);
    }
}

void AbeMovementComponent::PreStandingToRunning(AbeMovementComponent::States)
{
    SetXSpeed(kAbeRunSpeed);
    SetAnimation(AbeAnimation::eAbeStandToRun);
}

void AbeMovementComponent::StandingToRunning()
{
    if (mAnimationComponent->Complete())
    {
        SetState(States::eRunning);
    }
}

void AbeMovementComponent::PreStandingToCrouching(AbeMovementComponent::States)
{
    SetAnimation(AbeAnimation::eAbeStandToCrouch);
}

void AbeMovementComponent::StandingToCrouching()
{
    if (mAnimationComponent->Complete())
    {
        SetState(States::eCrouching);
    }
}

void AbeMovementComponent::PreStandingPushingWall(AbeMovementComponent::States)
{
    SetAnimation(AbeAnimation::eAbeStandPushWall);
}

void AbeMovementComponent::StandingPushingWall()
{
    if (mAnimationComponent->Complete())
    {
        SetState(States::eStanding);
    }
}

void AbeMovementComponent::PreStandingTurningAround(AbeMovementComponent::States)
{
    SetAnimation(AbeAnimation::eAbeStandTurnAround);
}

void AbeMovementComponent::StandingTurningAround()
{
    if (mAnimationComponent->Complete())
    {
        FlipDirection();
        SetState(States::eStanding);
    }
}

void AbeMovementComponent::PreWalking(AbeMovementComponent::States)
{
    SetAnimation(AbeAnimation::eAbeWalking);
    SetXSpeed(kAbeWalkSpeed);
}

void AbeMovementComponent::Walking()
{

    if (FrameIs(5 + 1) || FrameIs(14 + 1))
    {
        PlaySoundEffect("Abe_Step");
        SnapXToGrid();
        if (IsMovingTowardsWall())
        {
            GoToStandingPushingWallOrCrouching();
        }
        else
        {
            if (!DirectionChanged() && mData.mRunning && IsMovingLeftOrRight())
            {
                SetState(States::eWalkingToRunning);
            }
        }
    }
    else if (FrameIs(2 + 1) || FrameIs(11 + 1))
    {
        if (DirectionChanged() || !IsMovingLeftOrRight())
        {
            SetState(States::eWalkingToStanding);
        }
    }
}

void AbeMovementComponent::PreWalkingToRunning(AbeMovementComponent::States)
{
    SetXSpeed(kAbeRunSpeed);
    SetAnimation(FrameIs(5 + 1) ? AbeAnimation::eAbeWalkingToRunning : AbeAnimation::eAbeWalkingToRunningMidGrid);
}

void AbeMovementComponent::WalkingToRunning()
{
    if (mAnimationComponent->Complete())
    {
        SetState(States::eRunning);
    }
}

void AbeMovementComponent::PreWalkingToStanding(AbeMovementComponent::States)
{
    SetXSpeed(kAbeWalkSpeed);
    SetAnimation(FrameIs(2 + 1) ? AbeAnimation::eAbeWalkToStand : AbeAnimation::eAbeWalkToStandMidGrid);
}

void AbeMovementComponent::WalkingToStanding()
{
    if (FrameIs(2))
    {
        PlaySoundEffect("Abe_Step");
    }
    if (mAnimationComponent->Complete())
    {
        SetState(States::eStanding);
    }
}

void AbeMovementComponent::PreRunning(AbeMovementComponent::States)
{
    SetAnimation(AbeAnimation::eAbeRunning);
}

void AbeMovementComponent::Running()
{
    if (IsMovingTowardsWall())
    {
        SnapXToGrid();
        SetState(States::eFallingBack);
    }
    if (FrameIs(0 + 1) || FrameIs(8 + 1))
    {
        SnapXToGrid();
    }
    if (FrameIs(4 + 1) || FrameIs(12 + 1))
    {
        SnapXToGrid();

        if (IsMovingLeftOrRight())
        {
            if (DirectionChanged())
            {
                SetState(States::eRunningTurningAround);
            }
            else if (!mData.mRunning)
            {
                SetState(States::eRunningToWalking);
            }
        }
        else
        {
            SetState(States::eRunningToStanding);
        }
    }
}

void AbeMovementComponent::PreRunningToWalking(AbeMovementComponent::States)
{
    SetXSpeed(kAbeWalkSpeed);
    SetAnimation(FrameIs(2 + 1) ? AbeAnimation::eAbeRunningToWalk : AbeAnimation::eAbeRunningToWalkingMidGrid);
}

void AbeMovementComponent::RunningToWalking()
{
    if (mAnimationComponent->Complete())
    {
        SetState(States::eWalking);
    }
}

void AbeMovementComponent::PreRunningToStanding(AbeMovementComponent::States)
{
    SetXSpeed(3.0f); // TODO: approximation, handle velocity SetXVelocity(0.375)
    SetAnimation(AbeAnimation::eAbeRunningSkidStop);
}

void AbeMovementComponent::RunningToStanding()
{
    if (mAnimationComponent->Complete())
    {
        SnapXToGrid();
        SetState(States::eStanding);
    }
}

void AbeMovementComponent::PreRunningTurningAround(AbeMovementComponent::States)
{
    SetXSpeed(3.0f); // TODO: approximation, handle velocity SetXVelocity(0.375)
    SetAnimation(AbeAnimation::eAbeRunningToSkidTurn);
}

void AbeMovementComponent::RunningTurningAround()
{
    if (mAnimationComponent->Complete())
    {
        SnapXToGrid();
        if (mData.mRunning)
        {
            SetState(States::eRunningTurningAroundToRunning);
        }
        else
        {
            SetState(States::eRunningTurningAroundToWalking);
        }
    }
}

void AbeMovementComponent::PreRunningTurningAroundToWalking(AbeMovementComponent::States previous)
{
    SetXSpeed(-kAbeWalkSpeed);
    SetAnimation(AbeAnimation::eAbeRunningTurnAroundToWalk);
}

void AbeMovementComponent::RunningTurningAroundToWalking()
{
    if (mAnimationComponent->Complete())
    {
        FlipDirection();
        SetState(States::eWalking);
    }
}

void AbeMovementComponent::PreRunningTurningAroundToRunning(AbeMovementComponent::States)
{
    SetXSpeed(-kAbeRunSpeed);
    SetAnimation(AbeAnimation::eAbeRunningTurnAround);
}

void AbeMovementComponent::RunningTurningAroundToRunning()
{
    if (mAnimationComponent->Complete())
    {
        FlipDirection();
        SetState(States::eRunning);
    }
}

void AbeMovementComponent::PreCrouching(AbeMovementComponent::States)
{
    SetXSpeed(0.0f);
    SetAnimation(AbeAnimation::eAbeCrouchIdle);
}

void AbeMovementComponent::Crouching()
{
    if (IsMovingLeftOrRight())
    {
        if (DirectionChanged())
        {
            SetState(States::eCrouchingTurningAround);
        }
        else
        {
            SetState(States::eCrouchingToRolling);
        }
    }
    else if (mData.mGoal == Goal::eGoUp && !IsBelowCeiling())
    {
        SetState(States::eCrouchingToStanding);
    }
}

void AbeMovementComponent::PreCrouchingToRolling(AbeMovementComponent::States)
{
    SetXSpeed(kAbeRunSpeed);
    SetAnimation(AbeAnimation::eAbeCrouchToRoll);
}

void AbeMovementComponent::CrouchingToRolling()
{
    SetState(States::eRolling);
}

void AbeMovementComponent::PreCrouchingToStanding(AbeMovementComponent::States)
{
    SetAnimation(AbeAnimation::eAbeCrouchToStand);
}

void AbeMovementComponent::CrouchingToStanding()
{
    if (mAnimationComponent->Complete())
    {
        SetState(States::eStanding);
    }
}

void AbeMovementComponent::PreCrouchingTurningAround(AbeMovementComponent::States)
{
    SetAnimation(AbeAnimation::eAbeCrouchTurnAround);
}

void AbeMovementComponent::CrouchingTurningAround()
{
    if (mAnimationComponent->Complete())
    {
        FlipDirection();
        SetState(States::eCrouching);
    }
}

void AbeMovementComponent::PreRolling(States)
{
    SetAnimation(AbeAnimation::eAbeRolling);
}

void AbeMovementComponent::Rolling()
{
    if (FrameIs(0 + 1) || FrameIs(4 + 1) || FrameIs(8 + 1))
    {
        SnapXToGrid();
        if (!IsMovingLeftOrRight() || DirectionChanged())
        {
            SetState(States::eCrouching); // TODO: rolling to crouching
        }

    }
    else if (FrameIs(1 + 1) || FrameIs(5 + 1) || FrameIs(9 + 1))
    {
        if (!IsBelowCeiling() && (DirectionChanged() || mData.mRunning)) // TODO: check if run input is maybe buffered -here-?
        {
            SetState(States::eRollingToWalkingOrRunning);
        }
    }
}

void AbeMovementComponent::PreRollingToWalkingOrRunning(AbeMovementComponent::States previous)
{
    SetXSpeed(kAbeRunSpeed);
    SetAnimation(AbeAnimation::eAbeRunning); // TODO: get correct animation from hok
}

void AbeMovementComponent::RollingToWalkingOrRunning()
{
    if (FrameIs(0 + 1))
    {
        if (!mData.mRunning)
        {
            SetState(States::eStandingToWalking);
        }
        else
        {
            SetState(States::eStandingToRunning);
        }
    }
}

void AbeMovementComponent::PreFallingBack(AbeMovementComponent::States)
{
    SetXSpeed(0.0f);
    SetAnimation(AbeAnimation::eAbeFallBackStanding);
}

void AbeMovementComponent::FallingBack()
{
    if (mAnimationComponent->Complete())
    {
        SetState(States::eFallingBackToStanding);
    }
}

void AbeMovementComponent::PreFallingBackToStanding(States)
{
    SetAnimation(AbeAnimation::eAbeFallBackToStand);
}

void AbeMovementComponent::FallingBackToStanding()
{
    if (mAnimationComponent->Complete())
    {
        // TODO: play sound angry
        SetState(States::eFallingBackToStandingAngry);
    }
}

void AbeMovementComponent::PreFallingBackToStandingAngry(States)
{
    SetAnimation(AbeAnimation::eAbeStandSpeak5);
}

void AbeMovementComponent::FallingBackToStandingAngry()
{
    if (mAnimationComponent->Complete() || (FrameIs(1 + 1) && DirectionChanged()))
    {
        SetState(States::eStanding);
    }
}

void AbeMovementComponent::PreChanting(AbeMovementComponent::States)
{
    SetAnimation(AbeAnimation::eAbeStandToChant);
}

void AbeMovementComponent::Chanting()
{
    if (mData.mGoal == Goal::eStand)
    {
        SetState(States::eChantingToStanding);
    }
    else if (mData.mGoal == Goal::eChant) // Still chanting?
    {
        auto sligs = mEntity.GetManager()->With<SligMovementComponent>();
        if (!sligs.empty())
        {
            for (auto& slig : sligs)
            {
                LOG_INFO("Found a Slig to possess");
                slig.Destroy();
            }
        }
    }
}

void AbeMovementComponent::PreChantingToStanding(AbeMovementComponent::States)
{
    SetAnimation(AbeAnimation::eAbeChantToStand);
}

void AbeMovementComponent::ChantingToStanding()
{
    SetState(States::eStanding);
}

void AbeMovementComponent::GoToStandingPushingWallOrCrouching()
{
    if (mCollisionSystem->WallCollision(mAnimationComponent->mFlipX, mTransformComponent->GetX(), mTransformComponent->GetY(), 25.0f, -50.0f))
    {
        SetXSpeed(0.0f);
        if (mCollisionSystem->WallCollision(mAnimationComponent->mFlipX, mTransformComponent->GetX(), mTransformComponent->GetY(), 25.0f, -20.0f))
        {
            SetState(States::eStandingPushingWall);
        }
        else
        {
            SetState(States::eStandingToCrouching);
        }
    }
}

/**
 * Abe Movement Finite State Machine helpers
 */

bool AbeMovementComponent::IsBelowCeiling() const
{
    return static_cast<bool>(mCollisionSystem->CeilingCollision(mData.mDirection == Direction::eLeft, mTransformComponent->GetX(), mTransformComponent->GetY() - 2.0f, 0.0f, -60.0f));
}

bool AbeMovementComponent::DirectionChanged() const
{
    return (mData.mGoal == Goal::eGoLeft && mData.mDirection == Direction::eRight) || (mData.mGoal == Goal::eGoRight && mData.mDirection == Direction::eLeft);
}

bool AbeMovementComponent::IsMovingLeftOrRight() const
{
    return mData.mGoal == Goal::eGoLeft || mData.mGoal == Goal::eGoRight;
}

bool AbeMovementComponent::IsMovingTowardsWall() const
{
    return static_cast<bool>(mCollisionSystem->WallCollision(mData.mDirection == Direction::eLeft, mTransformComponent->GetX(), mTransformComponent->GetY(), 25.0f, -50.0f));
}

bool AbeMovementComponent::FrameIs(u32 frame) const
{
    return mAnimationComponent->FrameNumber() == frame;
}

void AbeMovementComponent::SetFrame(u32 frame)
{
    mAnimationComponent->SetFrame(frame);
}

void AbeMovementComponent::SetXSpeed(f32 speed)
{
    mPhysicsComponent->SetXSpeed(mData.mDirection * speed);
}

void AbeMovementComponent::SnapXToGrid()
{
    LOG_INFO("SNAP X");
    mTransformComponent->SnapXToGrid();
}

void AbeMovementComponent::FlipDirection()
{
    mData.mDirection = -mData.mDirection;
    mAnimationComponent->mFlipX = !mAnimationComponent->mFlipX;
}

void AbeMovementComponent::SetState(AbeMovementComponent::States state)
{
    auto prevState = mData.mState;
    mData.mState = state;
    auto it = mStateFnMap.find(mData.mState);
    if (it != std::end(mStateFnMap))
    {
        if (it->second.mPreHandler)
        {
            it->second.mPreHandler(this, prevState);
        }
    }
}

void AbeMovementComponent::SetAnimation(AbeAnimation abeAnimation, u32 animationFrame)
{
    mAnimationComponent->Change(kAbeAnimations.at(abeAnimation));
    SetFrame(animationFrame);
    mData.mAnimation = abeAnimation;
    mData.mAnimationFrame = animationFrame;
}

void AbeMovementComponent::PlaySoundEffect(const char* fxName)
{
    LOG_WARNING("TODO: Play: " << fxName);
    mEntity.GetManager()->GetSystem<SoundSystem>()->PlaySoundEffect(fxName);
}

void AbeMovementComponent::ToggleCheatMode()
{
    mData.mCheatEnabled = !mData.mCheatEnabled;
    mPhysicsComponent->SetSpeed(0.0f, 0.0f);
}

/**
 * Abe Player Controller (TODO: move in own file)
 */

DEFINE_COMPONENT(AbePlayerControllerComponent);

void AbePlayerControllerComponent::OnResolveDependencies()
{
    mGameCommands = &mEntity.GetManager()->GetSystem<InputSystem>()->Mapping();
    mAbeMovement = mEntity.GetComponent<AbeMovementComponent>();
}

void AbePlayerControllerComponent::Update()
{
    mAbeMovement->mData.mRunning = mGameCommands->PressedOrHeld(InputCommands::eRun) != 0;
    mAbeMovement->mData.mSneaking = mGameCommands->PressedOrHeld(InputCommands::eSneak) != 0;
    if (mGameCommands->PressedOrHeld(InputCommands::eLeft) && !mGameCommands->PressedOrHeld(InputCommands::eRight))
    {
        mAbeMovement->mData.mGoal = AbeMovementComponent::Goal::eGoLeft;
    }
    else if (mGameCommands->PressedOrHeld(InputCommands::eRight) && !mGameCommands->PressedOrHeld(InputCommands::eLeft))
    {
        mAbeMovement->mData.mGoal = AbeMovementComponent::Goal::eGoRight;
    }
    else if (mGameCommands->PressedOrHeld(InputCommands::eDown) && !mGameCommands->PressedOrHeld(InputCommands::eUp))
    {
        mAbeMovement->mData.mGoal = AbeMovementComponent::Goal::eGoDown;
    }
    else if (mGameCommands->PressedOrHeld(InputCommands::eUp) && !mGameCommands->PressedOrHeld(InputCommands::eDown))
    {
        mAbeMovement->mData.mGoal = AbeMovementComponent::Goal::eGoUp;
    }
    else if (mGameCommands->PressedOrHeld(InputCommands::eChant))
    {
        mAbeMovement->mData.mGoal = AbeMovementComponent::Goal::eChant;
    }
    else if (mGameCommands->Pressed(InputCommands::eCheatMode))
    {
        mAbeMovement->ToggleCheatMode();
    }
    else
    {
        mAbeMovement->mData.mGoal = AbeMovementComponent::Goal::eStand;
    }
}