#include "core/entitymanager.hpp"
#include "core/systems/resourcesystem.hpp"
#include "core/components/transformcomponent.hpp"
#include "core/components/animationcomponent.hpp"

DEFINE_COMPONENT(AnimationComponent);

void AnimationComponent::OnLoad()
{
    Component::OnLoad(); // calls OnResolveDependencies

    mResourceLocator = mEntity->GetManager()->GetSystem<ResourceSystem>()->GetResourceLocator();
}

void AnimationComponent::OnResolveDependencies()
{
    mTransformComponent = mEntity->GetComponent<TransformComponent>();
}

void AnimationComponent::Load(const char* animationName)
{
    mCachedAnimations[animationName] = mResourceLocator->LocateAnimation(animationName).get();
}

void AnimationComponent::Change(const char* animationName)
{
    auto found = mCachedAnimations.find(animationName);
    if (found != mCachedAnimations.end())
    {
        mAnimation = found->second.get();
    }
    else
    {
        Load(animationName);
        mAnimation = mCachedAnimations[animationName].get();
    }
    mAnimation->SetFrame(0);
    mLoaded = true;
}

bool AnimationComponent::Complete() const
{
    return mAnimation->IsComplete();
}

u32 AnimationComponent::FrameNumber() const
{
    return static_cast<u32>(mAnimation->FrameNumber());
}

void AnimationComponent::SetFrame(u32 frame)
{
    mAnimation->SetFrame(frame);
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