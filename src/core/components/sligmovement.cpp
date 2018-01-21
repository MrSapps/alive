#include "core/entity.hpp"
#include "core/components/physics.hpp"
#include "core/components/animation.hpp"
#include "core/components/sligmovement.hpp"

void SligMovementComponent::Load()
{
    mPhysicsComponent = mEntity->GetComponent<PhysicsComponent>(ComponentIdentifier::Physics);
    mAnimationComponent = mEntity->GetComponent<AnimationComponent>(ComponentIdentifier::Animation);
}

void SligPlayerControllerComponent::Load(const InputState& state)
{
    mInputMappingActions = &state.Mapping().GetActions(); // TODO: Input is wired here
    mSligMovement = mEntity->GetComponent<SligMovementComponent>(ComponentIdentifier::SligMovementController);
}

void SligPlayerControllerComponent::Update()
{
    mSligMovement->mLeft = mInputMappingActions->Left(mInputMappingActions->mIsDown);
    mSligMovement->mRight = mInputMappingActions->Right(mInputMappingActions->mIsDown);
}