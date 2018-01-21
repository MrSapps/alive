#include "core/entity.hpp"
#include "core/components/animation.hpp"

void AnimationComponent::Load(ResourceLocator& resLoc, const char* animationName)
{
    mAnimation = resLoc.LocateAnimation(animationName).get();
    mTransformComponent = mEntity->GetComponent<TransformComponent>(ComponentIdentifier::Transform);
}

void AnimationComponent::Render(AbstractRenderer& rend) const
{
    mAnimation->Render(rend,
                       false,
                       AbstractRenderer::eLayers::eForegroundLayer1,
                       static_cast<s32>(mTransformComponent->xPos),
                       static_cast<s32>(mTransformComponent->yPos));
}

void AnimationComponent::Update()
{
    mAnimation->Update();
}

void PhysicsComponent::Load()
{
    mTransformComponent = mEntity->GetComponent<TransformComponent>(ComponentIdentifier::Transform);
}

void PhysicsComponent::Update()
{
    mTransformComponent->xPos += xSpeed;
    mTransformComponent->yPos += ySpeed;
}

void AbeMovementControllerComponent::Load()
{
    mPhysicsComponent = mEntity->GetComponent<PhysicsComponent>(ComponentIdentifier::Physics);
}

void AbeMovementControllerComponent::Update()
{
    if (left)
    {
        mPhysicsComponent->xSpeed = -0.5f;
    }
    else if (right)
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
    mAbeMovement = mEntity->GetComponent<AbeMovementControllerComponent>(ComponentIdentifier::AbeMovementController);
}

void PlayerControllerComponent::Update()
{
    mAbeMovement->left = mInputMappingActions->Left(mInputMappingActions->mIsDown);
    mAbeMovement->right = mInputMappingActions->Right(mInputMappingActions->mIsDown);
}
