#include "core/entity.hpp"
#include "core/entitymanager.hpp"
#include "core/components/physics.hpp"
#include "core/components/transform.hpp"
#include "core/components/animation.hpp"
#include "core/components/abemovement.hpp"
#include "core/components/sligmovement.hpp"

DEFINE_COMPONENT(AbeMovementComponent);

void AbeMovementComponent::SetTransistionData(const TransistionData* data)
{
    if (mNextTransistionData != data)
    {
        mAnimationComponent->Change(data->mAnimation);
        SetXSpeed(data->mXSpeed);
        mNextTransistionData = data;
    }
}

void AbeMovementComponent::Load()
{
    mPhysicsComponent = mEntity->GetComponent<PhysicsComponent>();
    mAnimationComponent = mEntity->GetComponent<AnimationComponent>();

    mStateFnMap[States::eStanding] = [&]() 
    {
        if (mGoal == Goal::eGoLeft || mGoal == Goal::eGoRight)
        {
            if ((!mAnimationComponent->mFlipX && mGoal == Goal::eGoLeft) || (mAnimationComponent->mFlipX && mGoal == Goal::eGoRight))
            {
                SetTransistionData(&kTurnAround);
            }
            else
            {
                SetTransistionData(&kStandToWalk);
            }
        }
        else if (mGoal == Goal::eChant)
        {
            mState = States::eChanting;
            mAnimationComponent->Change("AbeStandToChant");
        }
    };

    mStateFnMap[States::eChanting] = [&]()
    {
        if (mGoal == Goal::eStand)
        {
            SetTransistionData(&kChantToStand);
        }
        else if (mGoal == Goal::eChant)
        {
            auto sligs = mEntity->GetManager()->With<SligMovementComponent>();
            if (!sligs.empty())
            {
                LOG_INFO("Found a slig to possess");
            }
        }
    };

    mStateFnMap[States::eWalking] = [&]()
    {
        LOG_INFO("SNAP CHECK FRAME " << mAnimationComponent->FrameNumber());
        if (mAnimationComponent->FrameNumber() == 5+1 || mAnimationComponent->FrameNumber() == 14+1)
        {
            LOG_INFO("SNAP ABE");
            mEntity->GetComponent<TransformComponent>()->SnapXToGrid();
        }

        if (!mAnimationComponent->mFlipX && mGoal == Goal::eGoLeft)
        {
            // Invert direction
        }
        else if (mAnimationComponent->mFlipX && mGoal == Goal::eGoRight)
        {
            // Invert direction
        }
        else if (mGoal != Goal::eGoLeft && mGoal != Goal::eGoRight)
        {
            if (mAnimationComponent->FrameNumber() == 2 + 1 || mAnimationComponent->FrameNumber() == 11 + 1)
            {
                mState = States::eWalkingToStanding;
                if (mAnimationComponent->FrameNumber() == 2 + 1)
                {
                    mAnimationComponent->Change("AbeWalkToStand");
                }
                else
                {
                    mAnimationComponent->Change("AbeWalkToStandMidGrid");
                }
            }
        }
    };

    mStateFnMap[States::eWalkingToStanding] = [&]()
    {
        if (mAnimationComponent->Complete())
        {
            mAnimationComponent->Change("AbeStandIdle");
            mPhysicsComponent->xSpeed = 0.0f;
            mState = States::eStanding;
        }
    };
}

void AbeMovementComponent::Update()
{
    if (mNextTransistionData && mAnimationComponent->Complete())
    {
        if (mNextTransistionData->mFlipDirection)
        {
            mAnimationComponent->mFlipX = !mAnimationComponent->mFlipX;
        }
        
        if (mNextTransistionData->mNextAnimation)
        {
            mAnimationComponent->Change(mNextTransistionData->mNextAnimation);
        }

        mState = mNextTransistionData->mNextState;
        mNextTransistionData = nullptr;
    }

    mStateFnMap[mState]();
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