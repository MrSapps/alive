#pragma once

#include "core/component.hpp"
#include "resourcemapper.hpp"

class TransformComponent;

class AnimationComponent final : public Component
{
public:
    void Load(ResourceLocator& resLoc, const char* animationName);
    void Update() final;
    void Render(AbstractRenderer& rend) const final;
private:
    std::unique_ptr<Animation> mAnimation;
private:
    TransformComponent* mTransformComponent = nullptr;
};