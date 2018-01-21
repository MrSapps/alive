#include "core/entity.hpp"
#include "core/components/physics.hpp"
#include "core/components/animation.hpp"
#include "core/components/abemovement.hpp"


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
    mPhysicsComponent = mEntity->GetComponent<PhysicsComponent>(ComponentIdentifier::Physics);
    mAnimationComponent = mEntity->GetComponent<AnimationComponent>(ComponentIdentifier::Animation);

    mStateFnMap[States::eStanding] = [&]() 
    {
        mStateGoalFnMap[States::eStanding][Goal::eGoLeft] = [&]()
        {
            if (!mAnimationComponent->mFlipX)
            {
                SetTransistionData(&kTurnAround);
            }
            else
            {
                SetTransistionData(&kStandToWalk);
            }
        };

        mStateGoalFnMap[States::eStanding][Goal::eGoRight] = [&]()
        {
            if (mAnimationComponent->mFlipX)
            {
                SetTransistionData(&kTurnAround);
            }
            else
            {
                SetTransistionData(&kStandToWalk);
            }
        };

        mStateGoalFnMap[States::eStanding][Goal::eChant] = [&]()
        {
            mState = States::eChanting;
            mAnimationComponent->Change("AbeStandToChant");
        };

        auto it = mStateGoalFnMap[mState].find(mGoal);
        if (it != std::end(mStateGoalFnMap[mState]))
        {
            mStateGoalFnMap[mState][mGoal]();
        }
    };

    mStateFnMap[States::eChanting] = [&]()
    {
        mStateGoalFnMap[States::eStanding][Goal::eStand] = [&]()
        {
            mAnimationComponent->Change("AbeChantToStand");
            mState = States::eChantToStand;
        };

        mStateGoalFnMap[States::eStanding][Goal::eChant] = [&]()
        {
            auto sligs = mEntity->GetParent()->FindChildrenByComponent(ComponentIdentifier::SligMovementController);
            if (!sligs.empty())
            //for (auto const& slig : sligs)
            {
                // auto controller = slig->GetComponent(ComponentIdentifier::PlayerController);
                // controller.mActive = true; or controller.possess(this);
                LOG_INFO("Found a slig to possess");
            }
        };
        
        auto it = mStateGoalFnMap[mState].find(mGoal);
        if (it != std::end(mStateGoalFnMap[mState]))
        {
            mStateGoalFnMap[mState][mGoal]();
        }
    };

    mStateFnMap[States::eChantToStand] = [&]()
    {
        if (mAnimationComponent->Complete())
        {
            mAnimationComponent->Change("AbeStandIdle");
            mState = States::eStanding;
        }
    };

    mStateFnMap[States::eWalking] = [&]()
    {
        LOG_INFO("SNAP CHECK FRAME " << mAnimationComponent->FrameNumber());
        if (mAnimationComponent->FrameNumber() == 5+1 || mAnimationComponent->FrameNumber() == 14+1)
        {
            LOG_INFO("SNAP ABE");
            mPhysicsComponent->SnapXToGrid();
        }

        if (mGoal != Goal::eGoLeft && mGoal != Goal::eGoRight)
        {
            if (mAnimationComponent->FrameNumber() == 2 + 1 || mAnimationComponent->FrameNumber() == 11 + 1)
            {
                mState = States::eWalkingToStanding;
                if (mAnimationComponent->FrameNumber() == 2 +1)
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
    mAbeMovement = mEntity->GetComponent<AbeMovementComponent>(ComponentIdentifier::AbeMovementController);

}

void AbePlayerControllerComponent::Update()
{
    if (mInputMappingActions->Left(mInputMappingActions->mIsDown))
    {
        mAbeMovement->mGoal = AbeMovementComponent::Goal::eGoLeft;
    }
    else if (mInputMappingActions->Right(mInputMappingActions->mIsDown))
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