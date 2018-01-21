#include "core/entity.hpp"
#include "core/components/physics.hpp"
#include "core/components/animation.hpp"
#include "core/components/abemovement.hpp"

const f32 kWalkSpeed = 2.777771f;

void AbeMovementComponent::Load()
{
    mPhysicsComponent = mEntity->GetComponent<PhysicsComponent>(ComponentIdentifier::Physics);
    mAnimationComponent = mEntity->GetComponent<AnimationComponent>(ComponentIdentifier::Animation);

    mStateFnMap[States::eStanding] = [&]() 
    {
        switch (mGoal)
        {
        case AbeMovementComponent::Goal::eGoLeft:
            if (!mAnimationComponent->mFlipX)
            {
                mAnimationComponent->Change("AbeStandTurnAround");
                mState = States::eStandingTurnAround;
            }
            else
            {
                mAnimationComponent->Change("AbeWalkToStand");
                mState = States::eStandingToWalking;
                if (mAnimationComponent->mFlipX)
                {
                    mPhysicsComponent->xSpeed = -kWalkSpeed;
                }
                else
                {
                    mPhysicsComponent->xSpeed = kWalkSpeed;
                }
            }
            break;
        case AbeMovementComponent::Goal::eGoRight:
            if (mAnimationComponent->mFlipX)
            {
                mAnimationComponent->Change("AbeStandTurnAround");
                mState = States::eStandingTurnAround;
            }
            else
            {
                mAnimationComponent->Change("AbeWalkToStand");
                mState = States::eStandingToWalking;
                if (mAnimationComponent->mFlipX)
                {
                    mPhysicsComponent->xSpeed = -kWalkSpeed;
                }
                else
                {
                    mPhysicsComponent->xSpeed = kWalkSpeed;
                }
            }
            break;
        case AbeMovementComponent::Goal::eChant:
            mState = States::eChanting;
            mAnimationComponent->Change("AbeStandToChant");
            break;
        default:
            break;
        }
    };

    mStateFnMap[States::eStandingTurnAround] = [&]()
    {
        if (mAnimationComponent->Complete())
        {
            mAnimationComponent->mFlipX = !mAnimationComponent->mFlipX;
            mAnimationComponent->Change("AbeStandIdle");
            mState = States::eStanding;
        }
    };

    mStateFnMap[States::eChanting] = [&]()
    {
        switch (mGoal)
        {
        case Goal::eStand:
            mAnimationComponent->Change("AbeChantToStand");
            mState = States::eChantToStand;
            break;

        default:
        {
            auto sligs = mEntity->GetParent()->FindChildrenByComponent(ComponentIdentifier::SligMovementController);
            if (!sligs.empty())
                //for (auto const& slig : sligs)
            {
                // auto controller = slig->GetComponent(ComponentIdentifier::PlayerController);
                // controller.mActive = true; or controller.possess(this);
                LOG_INFO("Found a slig to possess");
                break;
            }
        }
        break;
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

    mStateFnMap[States::eStandingToWalking] = [&]()
    {
        if (mAnimationComponent->Complete())
        {
            mAnimationComponent->Change("AbeWalking");
            mState = States::eWalking;
        }
    };

    mStateFnMap[States::eWalking] = [&]()
    {
        switch (mGoal)
        {
        case Goal::eStand:
            if (mAnimationComponent->FrameNumber() == 3 || mAnimationComponent->FrameNumber() == 12)
            {
                mState = States::eWalkingToStanding;
                if (mAnimationComponent->FrameNumber() == 3)
                {
                    mAnimationComponent->Change("AbeWalkToStand");
                }
                else
                {
                    mAnimationComponent->Change("AbeWalkToStandMidGrid");
                }
            }
            break;

        default:
            if (mAnimationComponent->mFlipX)
            {
                mPhysicsComponent->xSpeed = -kWalkSpeed;
            }
            else
            {
                mPhysicsComponent->xSpeed = kWalkSpeed;
            }
            break;
        }
    };

    mStateFnMap[States::eWalking] = [&]()
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
    mStateFnMap[mState]();
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