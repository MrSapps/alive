#include "core/entity.hpp"
#include "core/components/physics.hpp"
#include "core/components/animation.hpp"
#include "core/components/sligmovement.hpp"

DEFINE_COMPONENT(SligMovementComponent);

void SligMovementComponent::Load()
{
    mPhysicsComponent = mEntity->GetComponent<PhysicsComponent>();
    mAnimationComponent = mEntity->GetComponent<AnimationComponent>();
}

void SligPlayerControllerComponent::Load(const InputState& state)
{
    mInputMappingActions = &state.Mapping().GetActions(); // TODO: Input is wired here
    mSligMovement = mEntity->GetComponent<SligMovementComponent>();
}

void SligPlayerControllerComponent::Update()
{
    mSligMovement->mLeft = mInputMappingActions->Left(mInputMappingActions->mIsDown);
    mSligMovement->mRight = mInputMappingActions->Right(mInputMappingActions->mIsDown);
}