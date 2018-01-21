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

    if (mChant)
    {
        auto sligs = mEntity->GetParent()->FindChildrenByComponent(ComponentIdentifier::SligMovementController);
        for (auto const& slig : sligs)
        {
            // auto controller = slig->GetComponent(ComponentIdentifier::PlayerController);
            // controller.mActive = true; or controller.possess(this);
            LOG_INFO("Found a slig to possess");
            (void) slig;
            break;
        }
    }

}

void AbePlayerControllerComponent::Load(const InputState& state)
{
    mInputMappingActions = &state.Mapping().GetActions(); // TODO: Input is wired here
    mAbeMovement = mEntity->GetComponent<AbeMovementComponent>(ComponentIdentifier::AbeMovementController);
}

void AbePlayerControllerComponent::Update()
{
    mAbeMovement->mLeft = mInputMappingActions->Left(mInputMappingActions->mIsDown);
    mAbeMovement->mRight = mInputMappingActions->Right(mInputMappingActions->mIsDown);
    mAbeMovement->mChant = mInputMappingActions->Chant(mInputMappingActions->mIsDown);
}