#include "core/entity.hpp"
#include "core/components/physics.hpp"
#include "core/components/abemovement.hpp"

void AbeMovementComponent::Load()
{
    mPhysicsComponent = mEntity->GetComponent<PhysicsComponent>(ComponentIdentifier::Physics);
}

void AbeMovementComponent::Update()
{
    if (mLeft)
    {
        mPhysicsComponent->xSpeed = -0.5f;
    }
    else if (mRight)
    {
        mPhysicsComponent->xSpeed = +0.5f;
    }
    else
    {
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