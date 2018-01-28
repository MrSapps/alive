#include <map>
#include <string>

#include "core/entity.hpp"
#include "core/entitymanager.hpp"
#include "core/systems/inputsystem.hpp"
#include "core/components/physicscomponent.hpp"
#include "core/components/transformcomponent.hpp"
#include "core/components/animationcomponent.hpp"
#include "core/components/abemovementcomponent.hpp"
#include "core/components/sligmovementcomponent.hpp"

DEFINE_COMPONENT(AbeMovementComponent);

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
    Component::OnLoad(); // calls OnResolveDependencies

    mStateFnMap[States::eStanding] = { &AbeMovementComponent::PreStanding, &AbeMovementComponent::Standing };
    mStateFnMap[States::eChanting] = { &AbeMovementComponent::PreChanting, &AbeMovementComponent::Chanting };
    mStateFnMap[States::eWalking] = { &AbeMovementComponent::PreWalking, &AbeMovementComponent::Walking };
    mStateFnMap[States::eStandTurningAround] = { nullptr, &AbeMovementComponent::StandTurnAround };
    mStateFnMap[States::eWalkingToStanding] = { nullptr, &AbeMovementComponent::WalkToStand };

    SetState(States::eStanding);
}

void AbeMovementComponent::OnResolveDependencies()
{
    mPhysicsComponent = mEntity->GetComponent<PhysicsComponent>();
    mAnimationComponent = mEntity->GetComponent<AnimationComponent>();
    mTransformComponent = mEntity->GetComponent<TransformComponent>();
}

void AbeMovementComponent::Serialize(std::ostream& os) const
{
    // static_assert(std::is_pod<decltype(mData)>::value);
    os.write(static_cast<const char*>(static_cast<const void*>(&mData)), sizeof(decltype(mData)));
}

void AbeMovementComponent::Deserialize(std::istream& is)
{
    // static_assert(std::is_pod<decltype(mData)>::value);
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

void AbeMovementComponent::ASyncTransition()
{
    if (mAnimationComponent->Complete())
    {
        SetState(mData.mNextState);
    }
}

void AbeMovementComponent::WalkToStand()
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

bool AbeMovementComponent::DirectionChanged() const
{
    return (!mAnimationComponent->mFlipX && mData.mGoal == Goal::eGoLeft) || (mAnimationComponent->mFlipX && mData.mGoal == Goal::eGoRight);
}

bool AbeMovementComponent::TryMoveLeftOrRight() const
{
    return mData.mGoal == Goal::eGoLeft || mData.mGoal == Goal::eGoRight;
}

void AbeMovementComponent::SetAnimation(const std::string& anim)
{
    mAnimationComponent->Change(anim.c_str());
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

void AbeMovementComponent::SetCurrentAndNextState(AbeMovementComponent::States current, AbeMovementComponent::States next)
{
    SetState(current);
    mData.mNextState = next;
}

void AbeMovementComponent::PreStanding(AbeMovementComponent::States /*previous*/)
{
    SetAnimation(kAbeAnimations.at(AbeAnimation::eAbeStandIdle));
    mPhysicsComponent->xSpeed = 0.0f;
    mPhysicsComponent->ySpeed = 0.0f;
}

void AbeMovementComponent::Standing()
{
    if (TryMoveLeftOrRight())
    {
        if (DirectionChanged())
        {
            SetAnimation(kAbeAnimations.at(AbeAnimation::eAbeStandTurnAround));
            SetCurrentAndNextState(States::eStandTurningAround, States::eStanding);
        }
        else
        {
            SetAnimation(kAbeAnimations.at(AbeAnimation::eAbeStandToWalk));
            SetXSpeed(kAbeWalkSpeed);
            SetCurrentAndNextState(States::eWalking, States::eStandToWalking);
        }
    }
    else if (mData.mGoal == Goal::eChant)
    {
        SetState(States::eChanting);
    }
}

void AbeMovementComponent::PreChanting(AbeMovementComponent::States /*previous*/)
{
    SetAnimation(kAbeAnimations.at(AbeAnimation::eAbeStandToChant));
}

void AbeMovementComponent::Chanting()
{
    if (mData.mGoal == Goal::eStand)
    {
        SetAnimation(kAbeAnimations.at(AbeAnimation::eAbeChantToStand));
        SetCurrentAndNextState(States::eChantToStand, States::eStanding);
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

void AbeMovementComponent::PreWalking(AbeMovementComponent::States /*previous*/)
{
    SetAnimation(kAbeAnimations.at(AbeAnimation::eAbeWalking));
    SetXSpeed(kAbeWalkSpeed);
}

void AbeMovementComponent::Walking()
{
    if (FrameIs(5 + 1) || FrameIs(14 + 1))
    {
        SnapXToGrid();
    }

    if (DirectionChanged() || !TryMoveLeftOrRight())
    {
        if (FrameIs(2 + 1) || FrameIs(11 + 1))
        {
            SetCurrentAndNextState(States::eWalkingToStanding, States::eStanding);
            SetAnimation(FrameIs(2 + 1) ? kAbeAnimations.at(AbeAnimation::eAbeWalkToStand) : kAbeAnimations.at(AbeAnimation::eAbeWalkToStandMidGrid));
        }
    }
}

void AbeMovementComponent::StandTurnAround()
{
    if (mAnimationComponent->Complete())
    {
        mAnimationComponent->mFlipX = !mAnimationComponent->mFlipX;
        SetState(States::eStanding);
    }
}

void AbeMovementComponent::SnapXToGrid()
{
    LOG_INFO("SNAP X");
    mTransformComponent->SnapXToGrid();
}

void AbeMovementComponent::SetXSpeed(f32 speed)
{
    if (mAnimationComponent->mFlipX)
    {
        mPhysicsComponent->xSpeed = -speed;
    }
    else
    {
        mPhysicsComponent->xSpeed = speed;
    }
}

bool AbeMovementComponent::FrameIs(u32 frame) const
{
    return mAnimationComponent->FrameNumber() == frame;
}

void AbeMovementComponent::SetFrame(u32 frame)
{
    mAnimationComponent->SetFrame(frame);
}

void AbeMovementComponent::PlaySoundEffect(const char* fxName)
{
    // TODO
    LOG_WARNING("TODO: Play: " << fxName);
}

DEFINE_COMPONENT(AbePlayerControllerComponent);

void AbePlayerControllerComponent::OnResolveDependencies()
{

    mEntity->GetManager()->With<InputSystem>([this](auto, auto inputSystem)
                                             {
                                                 mInputMappingActions = inputSystem->GetActions();
                                             });
    mAbeMovement = mEntity->GetComponent<AbeMovementComponent>();
}

void AbePlayerControllerComponent::Update()
{
    if (mInputMappingActions->Left(mInputMappingActions->mIsDown) && !mInputMappingActions->Right(mInputMappingActions->mIsDown))
    {
        mAbeMovement->mData.mGoal = AbeMovementComponent::Goal::eGoLeft;
    }
    else if (mInputMappingActions->Right(mInputMappingActions->mIsDown) && !mInputMappingActions->Left(mInputMappingActions->mIsDown))
    {
        mAbeMovement->mData.mGoal = AbeMovementComponent::Goal::eGoRight;
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