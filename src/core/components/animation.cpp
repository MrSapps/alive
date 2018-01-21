#include "core/entity.hpp"
#include "core/components/transform.hpp"
#include "core/components/animation.hpp"

void AnimationComponent::Load(ResourceLocator& resLoc, const char* animationName)
{
    mAnimation = resLoc.LocateAnimation(animationName).get();
    mAnimation->SetFrame(0);
    mResourceLocator = &resLoc;
    mTransformComponent = mEntity->GetComponent<TransformComponent>(ComponentIdentifier::Transform);
}

void AnimationComponent::Change(const char* animationName)
{
    mAnimation = mResourceLocator->LocateAnimation(animationName).get();
    mAnimation->SetFrame(0);
}

bool AnimationComponent::Complete() const
{
    return mAnimation->IsComplete();
}

s32 AnimationComponent::FrameNumber() const
{
    return mAnimation->FrameNumber();
}

void AnimationComponent::Render(AbstractRenderer& rend) const
{
    mAnimation->Render(rend,
                       mFlipX,
                       AbstractRenderer::eLayers::eForegroundLayer1,
                       static_cast<s32>(mTransformComponent->GetX()),
                       static_cast<s32>(mTransformComponent->GetY()));
}
void AnimationComponent::Update()
{
    mAnimation->Update();
}