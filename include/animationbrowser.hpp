#pragma once

#include "engine.hpp"
#include "core/entitymanager.hpp"

class Animation;

class AnimationBrowser
{
public:
    AnimationBrowser(EntityManager& entityManager, ResourceLocator& resMapper);

public:
    AnimationBrowser(AnimationBrowser&&) = delete;
    AnimationBrowser& operator=(AnimationBrowser&&) = delete;

public:
    void Render(AbstractRenderer& renderer);
    void Update(const InputReader& input, CoordinateSpace& coords);

private:
    void RenderAnimationSelector(CoordinateSpace& coords);

private:
    s32 mXDelta = 0;
    s32 mYDelta = 0;
    bool mDebugResetAnimStates = false;
    Entity mSelected = nullptr; // Not owned
    EntityManager& mEntityManager;
    ResourceLocator& mResourceLocator;
    std::vector<Entity> mLoadedAnims;
};
