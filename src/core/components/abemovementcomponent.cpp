#include "core/entity.hpp"
#include "core/entitymanager.hpp"
#include "core/systems/inputsystem.hpp"
#include "core/systems/collisionsystem.hpp"
#include "core/components/physicscomponent.hpp"
#include "core/components/transformcomponent.hpp"
#include "core/components/animationcomponent.hpp"
#include "core/components/abemovementcomponent.hpp"
#include "core/components/sligmovementcomponent.hpp"

DEFINE_COMPONENT(AbeMovementComponent);

static const f32 kAbeRunSpeed = 6.25;
static const f32 kAbeWalkSpeed = 2.777771f;
static const std::map<AbeMovementComponent::AbeAnimation, std::string> kAbeAnimations = {
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
    { AbeMovementComponent::AbeAnimation::eAbeGassed, std::string{ "AbeGassed" }}
};

void AbeMovementComponent::OnLoad()
{
    mStateFnMap[States::ePushingWall] = { &AbeMovementComponent::PrePushingWall, &AbeMovementComponent::PushingWall };

    mStateFnMap[States::eStanding] = { &AbeMovementComponent::PreStanding, &AbeMovementComponent::Standing };
    mStateFnMap[States::eStandingTurningAround] = { nullptr, &AbeMovementComponent::StandingTurningAround };

    mStateFnMap[States::eRunning] = { &AbeMovementComponent::PreRunning, &AbeMovementComponent::Running };
    mStateFnMap[States::eRunningToStanding] = { nullptr, &AbeMovementComponent::RunningToStanding };
    mStateFnMap[States::eRunningTurningAround] = { nullptr, &AbeMovementComponent::RunningTurningAround };
    mStateFnMap[States::eRunningTurningAroundToWalking] = { nullptr, &AbeMovementComponent::RunningTurningAroundToWalking };
    mStateFnMap[States::eRunningTurningAroundToRunning] = { nullptr, &AbeMovementComponent::RunningTurningAroundToRunning };

    mStateFnMap[States::eWalking] = { &AbeMovementComponent::PreWalking, &AbeMovementComponent::Walking };
    mStateFnMap[States::eWalkingToStanding] = { nullptr, &AbeMovementComponent::WalkingToStanding };

    mStateFnMap[States::eChanting] = { &AbeMovementComponent::PreChanting, &AbeMovementComponent::Chanting };

    mStateFnMap[States::eCrouching] = { &AbeMovementComponent::PreCrouching, &AbeMovementComponent::Crouching };
    mStateFnMap[States::eCrouchingTurningAround] = { nullptr, &AbeMovementComponent::CrouchingTurningAround };

    Component::OnLoad(); // calls OnResolveDependencies
}

void AbeMovementComponent::OnResolveDependencies()
{
    mCollisionSystem = mEntity->GetManager()->GetSystem<CollisionSystem>();

    mPhysicsComponent = mEntity->GetComponent<PhysicsComponent>();
    mAnimationComponent = mEntity->GetComponent<AnimationComponent>();
    mTransformComponent = mEntity->GetComponent<TransformComponent>();

    for (auto const& animationName : kAbeAnimations)
    {
        mAnimationComponent->Load(animationName.second.c_str());
    }

    SetState(States::eStanding);
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
    auto it = mStateFnMap.find(mData.mState);
    if (it != std::end(mStateFnMap) && it->second.mHandler)
    {
        it->second.mHandler(this);
    }
    else
    {
        ASyncTransition();
    }
}

/**
 * Abe Movement Finite State Machine
 */

void AbeMovementComponent::PrePushingWall(AbeMovementComponent::States)
{
    SetAnimation(kAbeAnimations.at(AbeAnimation::eAbeStandPushWall));
}

void AbeMovementComponent::PushingWall()
{
    SetCurrentAndNextState(States::ePushingWallToStanding, States::eStanding);
}

void AbeMovementComponent::PreStanding(AbeMovementComponent::States)
{
    SetAnimation(kAbeAnimations.at(AbeAnimation::eAbeStandIdle));
    mPhysicsComponent->xSpeed = 0.0f;
    mPhysicsComponent->ySpeed = 0.0f;
}

void AbeMovementComponent::Standing()
{
    if (IsMovingLeftOrRight())
    {
        if (DirectionChanged())
        {
            SetAnimation(kAbeAnimations.at(AbeAnimation::eAbeStandTurnAround));
            SetCurrentAndNextState(States::eStandingTurningAround, States::eStanding);
        }
        else
        {
            if (IsMovingTowardsWall())
            {
                PushWallOrCrouch();
            }
            else
            {
                if (IsRunningLeftOrRight())
                {
                    StandingToRunning();
                }
                else
                {
                    StandingToWalking();
                }
            }
        }
    }
    else if (mData.mGoal == Goal::eGoDown)
    {
        StandingToCrouching();
    }
    else if (mData.mGoal == Goal::eChant)
    {
        SetState(States::eChanting);
    }
}

void AbeMovementComponent::StandingToWalking()
{
    SetXSpeed(kAbeWalkSpeed);
    SetAnimation(kAbeAnimations.at(AbeAnimation::eAbeStandToWalk));
    SetCurrentAndNextState(States::eStandingToWalking, States::eWalking);
}

void AbeMovementComponent::StandingToRunning()
{
    SetXSpeed(kAbeRunSpeed);
    SetAnimation(kAbeAnimations.at(AbeAnimation::eAbeStandToRun));
    SetCurrentAndNextState(States::eStandingToRunning, States::eRunning);
}

void AbeMovementComponent::StandingToCrouching()
{
    SetAnimation(kAbeAnimations.at(AbeAnimation::eAbeStandToCrouch));
    SetCurrentAndNextState(States::eStandingToCrouching, States::eCrouching);
}

void AbeMovementComponent::StandingTurningAround()
{
    if (mAnimationComponent->Complete())
    {
        mAnimationComponent->mFlipX = !mAnimationComponent->mFlipX;
        SetState(mData.mNextState);
    }
}

void AbeMovementComponent::Chanting()
{
    if (mData.mGoal == Goal::eStand)
    {
        SetAnimation(kAbeAnimations.at(AbeAnimation::eAbeChantToStand));
        SetCurrentAndNextState(States::eChantToStanding, States::eStanding);
    }
        // Still chanting?
    else if (mData.mGoal == Goal::eChant)
    {
        auto sligs = mEntity->GetManager()->With<SligMovementComponent>();
        if (!sligs.empty())
        {
            for (auto& slig : sligs)
            {
                LOG_INFO("Found a Slig to possess");
                slig->Destroy();
            }
        }
    }
}

void AbeMovementComponent::PreCrouching(AbeMovementComponent::States)
{
    SetAnimation(kAbeAnimations.at(AbeAnimation::eAbeCrouchIdle));
}

void AbeMovementComponent::PreWalking(AbeMovementComponent::States)
{
    SetAnimation(kAbeAnimations.at(AbeAnimation::eAbeWalking));
    SetXSpeed(kAbeWalkSpeed);
}

void AbeMovementComponent::Walking()
{
    if (FrameIs(5 + 1) || FrameIs(14 + 1))
    {
        SnapXToGrid();
        if (IsMovingTowardsWall())
        {
            PushWallOrCrouch();
        }
        else
        {
            if (!DirectionChanged() && IsRunningLeftOrRight())
            {
                SetXSpeed(kAbeRunSpeed);
                SetAnimation(FrameIs(5 + 1) ? kAbeAnimations.at(AbeAnimation::eAbeWalkingToRunning) : kAbeAnimations.at(AbeAnimation::eAbeWalkingToRunningMidGrid));
                SetCurrentAndNextState(States::eWalkingToRunning, States::eRunning);
            }
        }
    }
    else if (FrameIs(2 + 1) || FrameIs(11 + 1))
    {
        if (DirectionChanged() || !IsMovingLeftOrRight())
        {
            SetXSpeed(kAbeWalkSpeed);
            SetAnimation(FrameIs(2 + 1) ? kAbeAnimations.at(AbeAnimation::eAbeWalkToStand) : kAbeAnimations.at(AbeAnimation::eAbeWalkToStandMidGrid));
            SetCurrentAndNextState(States::eWalkingToStanding, States::eStanding);
        }
    }
}

void AbeMovementComponent::WalkingToStanding()
{
    if (FrameIs(2))
    {
        PlaySoundEffect("MOVEMENT_MUD_STEP");
    }
    if (mAnimationComponent->Complete())
    {
        SetState(mData.mNextState);
    }
}

void AbeMovementComponent::PreRunning(AbeMovementComponent::States)
{
    SetAnimation(kAbeAnimations.at(AbeAnimation::eAbeRunning));
}

void AbeMovementComponent::Running()
{
    if (FrameIs(0 + 1) || FrameIs(8 + 1))
    {
        SnapXToGrid();
    }
    if (FrameIs(4 + 1) || FrameIs(12 + 1))
    {
        SnapXToGrid();
        if (IsRunningLeftOrRight() && DirectionChanged())
        {
            SetAnimation(kAbeAnimations.at(AbeAnimation::eAbeRunningToSkidTurn));
            SetState(States::eRunningTurningAround);
        }
        else if (!IsRunningLeftOrRight())
        {
            if (IsMovingLeftOrRight())
            {
                SetXSpeed(kAbeWalkSpeed);
                SetAnimation(FrameIs(2 + 1) ? kAbeAnimations.at(AbeAnimation::eAbeRunningToWalk) : kAbeAnimations.at(AbeAnimation::eAbeRunningToWalkingMidGrid));
                SetCurrentAndNextState(States::eRunningToStanding, States::eWalking);
            }
            else
            {
                SetAnimation(kAbeAnimations.at(AbeAnimation::eAbeRunningSkidStop));
                SetCurrentAndNextState(States::eRunningToStanding, States::eStanding);
            }
        }
    }
}

void AbeMovementComponent::RunningToStanding()
{
    SetXSpeed(3.0f); // TODO: approximation, handle velocity
    if (mAnimationComponent->Complete())
    {
        SnapXToGrid();
        SetState(mData.mNextState);
    }
}

void AbeMovementComponent::RunningTurningAround()
{
    SetXSpeed(3.0f); // TODO: approximation, handle velocity
    if (mAnimationComponent->Complete())
    {
        SnapXToGrid();
        mData.mInvertX = true;
        if (IsRunningLeftOrRight())
        {
            SetXSpeed(kAbeRunSpeed);
            SetAnimation(kAbeAnimations.at(AbeAnimation::eAbeRunningTurnAround));
            SetCurrentAndNextState(States::eRunningTurningAroundToRunning, States::eRunning);
        }
        else
        {
            SetXSpeed(kAbeWalkSpeed);
            SetAnimation(kAbeAnimations.at(AbeAnimation::eAbeRunningTurnAroundToWalk));
            SetCurrentAndNextState(States::eRunningTurningAroundToWalking, States::eWalking);
        }
    }
}

void AbeMovementComponent::RunningTurningAroundToWalking()
{
    if (mAnimationComponent->Complete())
    {
        mData.mInvertX = false;
        mAnimationComponent->mFlipX = !mAnimationComponent->mFlipX;
        SetState(mData.mNextState);
    }
}

void AbeMovementComponent::RunningTurningAroundToRunning()
{
    if (mAnimationComponent->Complete())
    {
        mData.mInvertX = false;
        mAnimationComponent->mFlipX = !mAnimationComponent->mFlipX;
        SetState(mData.mNextState);
    }
}

void AbeMovementComponent::PreChanting(AbeMovementComponent::States)
{
    SetAnimation(kAbeAnimations.at(AbeAnimation::eAbeStandToChant));
}

void AbeMovementComponent::Crouching()
{
    if (IsMovingLeftOrRight())
    {
        if (DirectionChanged())
        {
            SetAnimation(kAbeAnimations.at(AbeAnimation::eAbeCrouchTurnAround));
            SetCurrentAndNextState(States::eCrouchingTurningAround, States::eCrouching);
        }
    }
    else if (mData.mGoal == Goal::eGoUp)
    {
        CrouchingToStanding();
    }
}

void AbeMovementComponent::CrouchingToStanding()
{
    SetAnimation(kAbeAnimations.at(AbeAnimation::eAbeCrouchToStand));
    SetCurrentAndNextState(States::eCrouchingToStanding, States::eStanding);
}

void AbeMovementComponent::CrouchingTurningAround()
{
    if (mAnimationComponent->Complete())
    {
        mAnimationComponent->mFlipX = !mAnimationComponent->mFlipX;
        SetState(mData.mNextState);
    }
}

void AbeMovementComponent::PushWallOrCrouch()
{
    if (mCollisionSystem->WallCollision(mAnimationComponent->mFlipX, mTransformComponent->GetX(), mTransformComponent->GetY(), 25, -50))
    {
        mPhysicsComponent->xSpeed = 0.0f;
        if (mCollisionSystem->WallCollision(mAnimationComponent->mFlipX, mTransformComponent->GetX(), mTransformComponent->GetY(), 25, -20))
        {
            SetState(States::ePushingWall);
        }
        else
        {
            StandingToCrouching();
        }
    }
}

/**
 * Abe Movement Finite State Machine helpers
 */

void AbeMovementComponent::ASyncTransition()
{
    if (mAnimationComponent->Complete())
    {
        SetState(mData.mNextState);
    }
}

bool AbeMovementComponent::DirectionChanged() const
{
    return (!mAnimationComponent->mFlipX && mData.mGoal == Goal::eGoLeft) || (mAnimationComponent->mFlipX && mData.mGoal == Goal::eGoRight) ||
        (!mAnimationComponent->mFlipX && mData.mGoal == Goal::eGoLeftRunning) || (mAnimationComponent->mFlipX && mData.mGoal == Goal::eGoRightRunning);
}

bool AbeMovementComponent::IsMovingLeftOrRight() const
{
    return mData.mGoal == Goal::eGoLeft || mData.mGoal == Goal::eGoRight || mData.mGoal == Goal::eGoLeftRunning || mData.mGoal == Goal::eGoRightRunning;
}

bool AbeMovementComponent::IsMovingTowardsWall() const
{
    return static_cast<bool>(mCollisionSystem->WallCollision(mAnimationComponent->mFlipX, mTransformComponent->GetX(), mTransformComponent->GetY(), 25, -50));
}

bool AbeMovementComponent::IsRunningLeftOrRight() const
{
    return mData.mGoal == Goal::eGoLeftRunning || mData.mGoal == Goal::eGoRightRunning;
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
    if (mAnimationComponent->mFlipX)
    {
        mPhysicsComponent->xSpeed = mData.mInvertX ? speed : -speed;
    }
    else
    {
        mPhysicsComponent->xSpeed = mData.mInvertX ? -speed : speed;
    }
}

void AbeMovementComponent::SnapXToGrid()
{
    LOG_INFO("SNAP X");
    mTransformComponent->SnapXToGrid();
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

void AbeMovementComponent::SetAnimation(const std::string& anim)
{
    mAnimationComponent->Change(anim.c_str());
}

void AbeMovementComponent::PlaySoundEffect(const char* fxName)
{
    // TODO
    LOG_WARNING("TODO: Play: " << fxName);
}

void AbeMovementComponent::SetCurrentAndNextState(AbeMovementComponent::States current, AbeMovementComponent::States next)
{
    SetState(current);
    mData.mNextState = next;
}

/**
 * Abe Player Controller (TODO: move in own file)
 */

DEFINE_COMPONENT(AbePlayerControllerComponent);

void AbePlayerControllerComponent::OnResolveDependencies()
{
    mInputMappingActions = mEntity->GetManager()->GetSystem<InputSystem>()->GetActions();
    mAbeMovement = mEntity->GetComponent<AbeMovementComponent>();
}

void AbePlayerControllerComponent::Update()
{
    if (mInputMappingActions->Left(mInputMappingActions->mIsDown) && !mInputMappingActions->Right(mInputMappingActions->mIsDown))
    {
        mAbeMovement->mData.mGoal = !mInputMappingActions->Run(mInputMappingActions->mIsDown) ? AbeMovementComponent::Goal::eGoLeft : AbeMovementComponent::Goal::eGoLeftRunning;
    }
    else if (mInputMappingActions->Right(mInputMappingActions->mIsDown) && !mInputMappingActions->Left(mInputMappingActions->mIsDown))
    {
        mAbeMovement->mData.mGoal = !mInputMappingActions->Run(mInputMappingActions->mIsDown) ? AbeMovementComponent::Goal::eGoRight : AbeMovementComponent::Goal::eGoRightRunning;
    }
    else if (mInputMappingActions->Down(mInputMappingActions->mIsDown) && !mInputMappingActions->Up(mInputMappingActions->mIsDown))
    {
        mAbeMovement->mData.mGoal = AbeMovementComponent::Goal::eGoDown;
    }
    else if (mInputMappingActions->Up(mInputMappingActions->mIsDown) && !mInputMappingActions->Down(mInputMappingActions->mIsDown))
    {
        mAbeMovement->mData.mGoal = AbeMovementComponent::Goal::eGoUp;
    }
    else if (mInputMappingActions->Chant(mInputMappingActions->mIsDown))
    {
        mAbeMovement->mData.mGoal = AbeMovementComponent::Goal::eChant;
    }
    else
    {
        mAbeMovement->mData.mGoal = AbeMovementComponent::Goal::eStand;
    }
}