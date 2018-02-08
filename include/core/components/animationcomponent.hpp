#pragma once

#include <memory>
#include <string>
#include <unordered_map>

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
    void Load(const std::string& animationName);
    void Load(const std::string& animationName, const std::string& dataSetName);
    void Change(const std::string& animationName);
    void Change(const std::string& animationName, const std::string& dataSetName);
    bool Complete() const;

public:
    u32 FrameNumber() const;
    void SetFrame(u32 frame);

public:
    bool mFlipX = false;
    bool mLoaded = false;
    Animation* mAnimation = nullptr;
    std::unordered_map<std::string, std::unique_ptr<Animation>> mCachedAnimations;

private:
    ResourceLocator* mResourceLocator = nullptr;
    TransformComponent* mTransformComponent = nullptr;
}; 
