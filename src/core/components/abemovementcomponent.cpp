#include "core/entity.hpp"
#include "core/entitymanager.hpp"
#include "core/systems/inputsystem.hpp"
#include "core/components/physicscomponent.hpp"
#include "core/components/transformcomponent.hpp"
#include "core/components/animationcomponent.hpp"
#include "core/components/abemovementcomponent.hpp"
#include "core/components/sligmovementcomponent.hpp"

DEFINE_COMPONENT(AbeMovementComponent);

void AbeMovementComponent::Deserialize(std::istream&)
{
    Load();
}

void AbeMovementComponent::Load()
{
    mPhysicsComponent = mEntity->GetComponent<PhysicsComponent>();
    mAnimationComponent = mEntity->GetComponent<AnimationComponent>();
    mTransformComponent = mEntity->GetComponent<TransformComponent>();

    mStateFnMap[States::eStanding] = { &AbeMovementComponent::PreStanding, &AbeMovementComponent::Standing };
    mStateFnMap[States::eChanting] = { &AbeMovementComponent::PreChanting, &AbeMovementComponent::Chanting };
    mStateFnMap[States::eWalking] = { &AbeMovementComponent::PreWalking, &AbeMovementComponent::Walking };
    mStateFnMap[States::eStandTurningAround] = { nullptr, &AbeMovementComponent::StandTurnAround };

    SetAnimation(kAbeStandIdleAnim);
}

bool AbeMovementComponent::DirectionChanged() const
{
    return (!mAnimationComponent->mFlipX && mGoal == Goal::eGoLeft) || (mAnimationComponent->mFlipX && mGoal == Goal::eGoRight);
}

bool AbeMovementComponent::TryMoveLeftOrRight() const
{
    return mGoal == Goal::eGoLeft || mGoal == Goal::eGoRight;
}

void AbeMovementComponent::SetAnimation(const AnimationData& anim)
{
    mAnimationComponent->Change(anim.mName);
}

void AbeMovementComponent::SetState(AbeMovementComponent::States state)
{
    auto prevState = mState;
    mState = state;
    auto it = mStateFnMap.find(mState);
    if (it != std::end(mStateFnMap))
    {
        if (it->second.mPreHandler)
        {
            it->second.mPreHandler(this, prevState);
        }
    }
}

void AbeMovementComponent::PreStanding(AbeMovementComponent::States /*previous*/)
{
    SetAnimation(kAbeStandIdleAnim);
    mPhysicsComponent->xSpeed = 0.0f;
    mPhysicsComponent->ySpeed = 0.0f;
}

void AbeMovementComponent::Standing()
{
    if (TryMoveLeftOrRight())
    {
        if (DirectionChanged())
        {
            SetAnimation(kAbeStandTurnAroundAnim);
            SetState(States::eStandTurningAround);
            mNextState = States::eStanding;
        }
        else
        {
            SetAnimation(kAbeStandToWalkAnim);
            mNextState = States::eWalking;
            SetXSpeed(kAbeWalkSpeed);
            SetState(States::eStandToWalking);
        }
    }
    else if (mGoal == Goal::eChant)
    {
        SetState(States::eChanting);
    }
}

void AbeMovementComponent::PreChanting(AbeMovementComponent::States /*previous*/)
{
    SetAnimation(kAbeStandToChantAnim);
}

void AbeMovementComponent::Chanting()
{
    if (mGoal == Goal::eStand)
    {
        SetAnimation(kAbeChantToStandAnim);
        mNextState = States::eStanding;
        SetState(States::eChantToStand);
    }
        // Still chanting?
    else if (mGoal == Goal::eChant)
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
    SetAnimation(kAbeWalkingAnim);
    SetXSpeed(kAbeWalkSpeed);
}

void AbeMovementComponent::Walking()
{
    if (mAnimationComponent->FrameNumber() == 5 + 1 || mAnimationComponent->FrameNumber() == 14 + 1)
    {
        mTransformComponent->SnapXToGrid();
    }

    if (DirectionChanged() || !TryMoveLeftOrRight())
    {
        if (mAnimationComponent->FrameNumber() == 2 + 1 || mAnimationComponent->FrameNumber() == 11 + 1)
        {
            SetState(States::eWalkingToStanding);
            mNextState = States::eStanding;
            SetAnimation(mAnimationComponent->FrameNumber() == 2 + 1 ? kAbeWalkToStandAnim1 : kAbeWalkToStandAnim2);
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

void AbeMovementComponent::ASyncTransition()
{
    if (mAnimationComponent->Complete())
    {
        SetState(mNextState);
    }
}

void AbeMovementComponent::Update()
{
    auto it = mStateFnMap.find(mState);
    if (it != std::end(mStateFnMap) && it->second.mHandler)
    {
        it->second.mHandler(this);
    }
    else
    {
        ASyncTransition();
    }
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

void AbePlayerControllerComponent::Deserialize(std::istream&)
{
    Load();
}

DEFINE_COMPONENT(AbePlayerControllerComponent);

void AbePlayerControllerComponent::Load()
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
        mAbeMovement->mGoal = AbeMovementComponent::Goal::eGoLeft;
    }
    else if (mInputMappingActions->Right(mInputMappingActions->mIsDown) && !mInputMappingActions->Left(mInputMappingActions->mIsDown))
    {
        mAbeMovement->mGoal = AbeMovementComponent::Goal::eGoRight;
    }
    else if (mInputMappingActions->Chant(mInputMappingActions->mIsDown))
    {
        mAbeMovement->mGoal = AbeMovementComponent::Goal::eChant;
    }
    else
    {
        mAbeMovement->mGoal = AbeMovementComponent::Goal::eStand;
    }
}