#pragma once

#include "core/component.hpp"
#include "resourcemapper.hpp"

class AnimationComponent : public Component
{
public:
    void Update() final;
    void Load(ResourceLocator& resLoc, const char* animationName);
    void Render(AbstractRenderer& rend) const final;
public:
    std::unique_ptr<Animation> mAnimation;
};

class PhysicsComponent : public Component
{
public:
    void Load();
    void Update() final;
public:
    void SnapXToGrid()
    {}
    void SnapYToGrid()
    {}
public:
    void SetX(float xPos);
    void SetY(float yPos);
private:
    bool FacingLeft()
    { return false; }
    float mXPos = 0.0f;
    float mYPos = 0.0f;
    bool mInvertX = false;
    float mXSpeed = 0.0f;
    float mXVelocity = 0.0f;
    float mYSpeed = 0.0f;
    float mYVelocity = 0.0f;
    AnimationComponent* mAnimationComponent;
};

class AbeControllerComponent : public Component
{
public:
    void Load();
    void Update() override;
private:
    PhysicsComponent* mPhysicsComponent;
};