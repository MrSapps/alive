#include "core/entity.hpp"

void Component::SetId(ComponentIdentifier id) {
    _id = id;
}

ComponentIdentifier Component::GetId() const {
    return _id;
}

AnimationComponent::AnimationComponent()
{

}

void AnimationComponent::Load(ResourceLocator& resLoc, const char* animationName)
{
    mAnimation = resLoc.LocateAnimation(animationName).get();
}

void AnimationComponent::Render(AbstractRenderer& rend) const
{
    mAnimation->Render(rend, false, AbstractRenderer::eLayers::eForegroundLayer1);
}

void AnimationComponent::Update()
{
    mAnimation->Update();
}

void PhysicsComponent::Update()
{
    if (mXSpeed > 0)
    {
        mXSpeed = mXSpeed - mXVelocity;
        if (FacingLeft())
        {
            if (mInvertX)
            {
                mXPos = mXPos + mXSpeed;
            }
            else
            {
                mXPos = mXPos - mXSpeed;
            }
        }
        else
        {
            if (mInvertX)
            {
                mXPos = mXPos - mXSpeed;
            }
            else
            {
                mXPos = mXPos + mXSpeed;
            }
        }
    }
}

Pawn::Pawn(ResourceLocator& resLoc) : mResourceLocator(resLoc)
{
    mPhysicsComponent = AddComponent<PhysicsComponent>(ComponentIdentifier::Physics);
    mAnimationComponent = AddComponent<AnimationComponent>(ComponentIdentifier::Animation);

    mAnimationComponent->Load(mResourceLocator, "AbeStandIdle");
}

void Pawn::Update()
{
    for (auto &component : mComponents)
    {
        component->Update();
    }
}

void Pawn::Render(AbstractRenderer& rend) const
{
    for (auto const &component : mComponents)
    {
        component->Render(rend);
    }
}
