#include "core/entitymanager.hpp"
#include "core/systems/resourcelocatorsystem.hpp"
#include "core/components/transformcomponent.hpp"
#include "core/components/animationcomponent.hpp"

DEFINE_COMPONENT(AnimationComponent);

void AnimationComponent::OnLoad()
{
    Component::OnLoad(); // calls OnResolveDependencies

    mEntity->GetManager()->With<ResourceLocatorSystem>([this](auto, auto resLoc)
                                                       {
                                                           mResourceLocator = resLoc->GetResourceLocator();
                                                       });
}

void AnimationComponent::OnResolveDependencies()
{
    mTransformComponent = mEntity->GetComponent<TransformComponent>();
}

void AnimationComponent::Change(const char* animationName)
{
    mAnimation = mResourceLocator->LocateAnimation(animationName).get();
    mAnimation->SetFrame(0);
    mLoaded = true;
}

bool AnimationComponent::Complete() const
{
    return mAnimation->IsComplete();
}

s32 AnimationComponent::FrameNumber() const
{
    return mAnimation->FrameNumber();
}

void AnimationComponent::Update()
{
    if (mLoaded)
    {
        mAnimation->Update();
    }
}

void AnimationComponent::Render(AbstractRenderer& rend) const
{
    if (mLoaded)
    {
        mAnimation->Render(rend,
                           mFlipX,
                           AbstractRenderer::eLayers::eForegroundLayer1,
                           static_cast<s32>(mTransformComponent->GetX()),
                           static_cast<s32>(mTransformComponent->GetY()));
    }
}