#pragma once

#include "resourcemapper.hpp"
#include "core/component.hpp"

class TransformComponent;

class AnimationComponent final : public Component
{
public:
    DECLARE_COMPONENT(AnimationComponent);

public:
    void OnLoad() final;
    void OnResolveDependencies() final;

public:
    void Update();
    void Render(AbstractRenderer& rend) const;

public:
    void Change(const char* animationName);
    bool Complete() const;
    u32 FrameNumber() const;
    void SetFrame(u32 frame);
public:
    bool mFlipX = false;
    bool mLoaded = false;
    std::unique_ptr<Animation> mAnimation;

private:
    ResourceLocator* mResourceLocator = nullptr;
    TransformComponent* mTransformComponent = nullptr;
}; 
