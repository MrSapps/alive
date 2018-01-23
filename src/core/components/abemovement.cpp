#include "core/entity.hpp"
#include "core/entitymanager.hpp"
#include "core/components/physics.hpp"
#include "core/components/transform.hpp"
#include "core/components/animation.hpp"
#include "core/components/abemovement.hpp"
#include "core/components/sligmovement.hpp"

DEFINE_COMPONENT(AbeMovementComponent);

bool AbeMovementComponent::DirectionChanged() const
{
    return !mAnimationComponent->mFlipX && mGoal == Goal::eGoLeft || mAnimationComponent->mFlipX && mGoal == Goal::eGoRight;
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
            SetXSpeed(kWalkSpeed);
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
            LOG_INFO("Found a Slig to possess");
        }
    }
}

void AbeMovementComponent::PreWalking(AbeMovementComponent::States /*previous*/)
{
    SetAnimation(kAbeWalkingAnim);
    SetXSpeed(kWalkSpeed);
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

void AbeMovementComponent::Load()
{
    mPhysicsComponent = mEntity->GetComponent<PhysicsComponent>();
    mAnimationComponent = mEntity->GetComponent<AnimationComponent>();
    mTransformComponent = mEntity->GetComponent<TransformComponent>();

    mStateFnMap[States::eStanding] =            { &AbeMovementComponent::PreStanding,  &AbeMovementComponent::Standing          };
    mStateFnMap[States::eChanting] =            { &AbeMovementComponent::PreChanting,  &AbeMovementComponent::Chanting          };
    mStateFnMap[States::eWalking] =             { &AbeMovementComponent::PreWalking,   &AbeMovementComponent::Walking           };
    mStateFnMap[States::eStandTurningAround] =  { nullptr,                             &AbeMovementComponent::StandTurnAround   };
}

void AbeMovementComponent::ASyncTransistion()
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
        ASyncTransistion();
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


void AbePlayerControllerComponent::Load(const InputState& state)
{
    mInputMappingActions = &state.Mapping().GetActions(); // TODO: Input is wired here
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
