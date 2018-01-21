#include "core/entity.hpp"
#include "core/components/physics.hpp"
#include "core/components/animation.hpp"
#include "core/components/abemovement.hpp"

void AbeMovementComponent::Load()
{
    mPhysicsComponent = mEntity->GetComponent<PhysicsComponent>(ComponentIdentifier::Physics);
    mAnimationComponent = mEntity->GetComponent<AnimationComponent>(ComponentIdentifier::Animation);
}

void AbeMovementComponent::Update()
{
    if (mLeft)
    {
        if (mPhysicsComponent->xSpeed == 0.0f)
        {
            mAnimationComponent->mFlipX = true;
            mAnimationComponent->Change("AbeWalking");
        }
        mPhysicsComponent->xSpeed = -2.5f;
    }
    else if (mRight)
    {
        if (mPhysicsComponent->xSpeed == 0.0f)
        {
            mAnimationComponent->mFlipX = false;

            mAnimationComponent->Change("AbeWalking");
        }
        mPhysicsComponent->xSpeed = +2.5f;
    }
    else
    {
        if (mPhysicsComponent->xSpeed != 0.0f)
        {
            mAnimationComponent->Change("AbeStandIdle");
        }
        mPhysicsComponent->xSpeed = 0.0f;
    }
}

void PlayerControllerComponent::Load(const InputState& state)
{
    mInputMappingActions = &state.Mapping().GetActions(); // TODO: Input is wired here
    mAbeMovement = mEntity->GetComponent<AbeMovementComponent>(ComponentIdentifier::AbeMovementController);
}

void PlayerControllerComponent::Update()
{
    mAbeMovement->mLeft = mInputMappingActions->Left(mInputMappingActions->mIsDown);
    mAbeMovement->mRight = mInputMappingActions->Right(mInputMappingActions->mIsDown);
}