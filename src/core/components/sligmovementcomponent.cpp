#include "core/entity.hpp"
#include "core/entitymanager.hpp"
#include "core/components/physicscomponent.hpp"
#include "core/components/transformcomponent.hpp"
#include "core/components/animationcomponent.hpp"
#include "core/components/sligmovementcomponent.hpp"

DEFINE_COMPONENT(SligMovementComponent);

void SligMovementComponent::Load()
{
    mPhysicsComponent = mEntity->GetComponent<PhysicsComponent>();
    mAnimationComponent = mEntity->GetComponent<AnimationComponent>();
    mTransformComponent = mEntity->GetComponent<TransformComponent>();

    mStateFnMap[States::eStanding] =            { &SligMovementComponent::PreStanding,  &SligMovementComponent::Standing          };
    mStateFnMap[States::eWalking] =             { &SligMovementComponent::PreWalking,   &SligMovementComponent::Walking           };
    mStateFnMap[States::eStandTurningAround] =  { nullptr,                              &SligMovementComponent::StandTurnAround   };
}

void SligMovementComponent::Update()
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

void SligMovementComponent::ASyncTransition()
{
    if (mAnimationComponent->Complete())
    {
        SetState(mNextState);
    }
}

bool SligMovementComponent::DirectionChanged() const
{
    return (!mAnimationComponent->mFlipX && mGoal == Goal::eGoLeft) || (mAnimationComponent->mFlipX && mGoal == Goal::eGoRight);
}

bool SligMovementComponent::TryMoveLeftOrRight() const
{
    return mGoal == Goal::eGoLeft || mGoal == Goal::eGoRight;
}

void SligMovementComponent::SetAnimation(const AnimationData& anim)
{
    mAnimationComponent->Change(anim.mName);
}

void SligMovementComponent::SetState(SligMovementComponent::States state)
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

void SligMovementComponent::PreStanding(SligMovementComponent::States /*previous*/)
{
    SetAnimation(kSligStandIdleAnim);
    mPhysicsComponent->xSpeed = 0.0f;
    mPhysicsComponent->ySpeed = 0.0f;
}

void SligMovementComponent::Standing()
{
    if (TryMoveLeftOrRight())
    {
        if (DirectionChanged())
        {
            SetAnimation(kSligStandTurnAroundAnim);
            SetState(States::eStandTurningAround);
            mNextState = States::eStanding;
        }
        else
        {
            SetAnimation(kSligStandToWalkAnim);
            mNextState = States::eWalking;
            SetXSpeed(kSligWalkSpeed);
            SetState(States::eStandToWalking);
        }
    }
    else if (mGoal == Goal::eChant)
    {
        SetState(States::eChanting);
    }
}

void SligMovementComponent::PreWalking(SligMovementComponent::States /*previous*/)
{
    SetAnimation(kSligWalkingAnim);
    SetXSpeed(kSligWalkSpeed);
}

void SligMovementComponent::Walking()
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
            SetAnimation(mAnimationComponent->FrameNumber() == 2 + 1 ? kSligWalkToStandAnim1 : kSligWalkToStandAnim2);
        }
    }
}

void SligMovementComponent::StandTurnAround()
{
    if (mAnimationComponent->Complete())
    {
        mTransformComponent->SnapXToGrid();
        mAnimationComponent->mFlipX = !mAnimationComponent->mFlipX;
        SetState(States::eStanding);
    }
}

void SligMovementComponent::SetXSpeed(f32 speed)
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


void SligPlayerControllerComponent::Load(const InputState& state)
{
    mInputMappingActions = &state.Mapping().GetActions(); // TODO: Input is wired here
    mSligMovement = mEntity->GetComponent<SligMovementComponent>();
}

void SligPlayerControllerComponent::Update()
{
    if (mInputMappingActions->Left(mInputMappingActions->mIsDown) && !mInputMappingActions->Right(mInputMappingActions->mIsDown))
    {
        mSligMovement->mGoal = SligMovementComponent::Goal::eGoLeft;
    }
    else if (mInputMappingActions->Right(mInputMappingActions->mIsDown) && !mInputMappingActions->Left(mInputMappingActions->mIsDown))
    {
        mSligMovement->mGoal = SligMovementComponent::Goal::eGoRight;
    }
    else if (mInputMappingActions->Chant(mInputMappingActions->mIsDown))
    {
        mSligMovement->mGoal = SligMovementComponent::Goal::eChant;
    }
    else
    {
        mSligMovement->mGoal = SligMovementComponent::Goal::eStand;
    }
}