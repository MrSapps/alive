#include "core/entity.hpp"
#include "core/components/animation.h"

void AnimationComponent::Load(ResourceLocator& resLoc, const char* animationName)
{
    mAnimation = resLoc.LocateAnimation(animationName).get();
    mTransformComponent = mEntity->GetComponent<TransformComponent>(ComponentIdentifier::Transform);
}

void AnimationComponent::Render(AbstractRenderer& rend) const
{
    mAnimation->Render(rend,
                       false,
                       static_cast<s32>(mTransformComponent->xPos),
                       static_cast<s32>(mTransformComponent->yPos),
                       AbstractRenderer::eLayers::eForegroundLayer1);
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

void AbeControllerComponent::Load()
{
    mPhysicsComponent = mEntity->GetComponent<PhysicsComponent>(ComponentIdentifier::Physics);
}

void AbeControllerComponent::Update()
{

}