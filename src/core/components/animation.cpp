#include "core/entity.hpp"
#include "core/components/animation.h"

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

void PhysicsComponent::Load()
{
    mAnimationComponent = mEntity->GetComponent<AnimationComponent>(ComponentIdentifier::Animation);
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
    mAnimationComponent->mAnimation->SetXPos(static_cast<s32>(mXPos));
    mAnimationComponent->mAnimation->SetYPos(static_cast<s32>(mYPos));
}

void PhysicsComponent::SetX(float xPos)
{
    mXPos = xPos;
}

void PhysicsComponent::SetY(float yPos)
{
    mYPos = yPos;
}

void AbeControllerComponent::Load()
{
    mPhysicsComponent = mEntity->GetComponent<PhysicsComponent>(ComponentIdentifier::Physics);
}

void AbeControllerComponent::Update()
{

}