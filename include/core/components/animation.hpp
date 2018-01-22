#pragma once

#include "core/component.hpp"
#include "resourcemapper.hpp"

class TransformComponent;

class AnimationComponent  : public Component
{
public:
    DECLARE_COMPONENT(AnimationComponent);
public:
    virtual ~AnimationComponent() = default;
    void Load(ResourceLocator& resLoc, const char* animationName);
    virtual void Change(const char* animationName);
    bool Complete() const;
    s32 FrameNumber() const;
    virtual void Update();
    void Render(AbstractRenderer& rend) const;
public:
    bool mFlipX = false;
    std::unique_ptr<Animation> mAnimation;
private:
    ResourceLocator* mResourceLocator = nullptr;
protected:
    TransformComponent* mTransformComponent = nullptr;
}; 

class AnimationComponentWithMeta : public AnimationComponent
{
public:
    DECLARE_COMPONENT(AnimationComponentWithMeta);

    void SetSnapXFrames(const std::vector<u32>* frames);
    virtual void Change(const char* animationName) override;
    virtual void Update() override;
private:
    const std::vector<u32>* mSnapXFrames = nullptr;
};
