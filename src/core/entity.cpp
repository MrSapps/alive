#include "core/entity.hpp"

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


void Pawn::Update()
{
    mPhysics.Update();
    mAnimation.Update();
}

void Pawn::Render(AbstractRenderer& rend) const
{
    mAnimation.Render(rend);
}

Pawn::Pawn(ResourceLocator& resLoc)
{
    mAnimation.Load(resLoc, "AbeStandIdle");
}

Pawn::~Pawn()
{

}
