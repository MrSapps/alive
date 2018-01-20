#pragma once

#include "resourcemapper.hpp"
#include <memory>

class AnimationComponent
{
public:
    AnimationComponent();

    void Update();
    void Load(ResourceLocator& resLoc, const char* animationName);
    void Render(AbstractRenderer& rend) const;
private:
    std::unique_ptr<Animation> mAnimation;
};

class PhysicsComponent
{
public:
    void Update();

    void SnapXToGrid() { }
    void SnapYToGrid() { }
private:
    bool FacingLeft() { return false; }
    float mXPos = 0.0f;
    float mYPos = 0.0f;
    bool mInvertX = false;
    float mXSpeed = 0.0f;
    float mXVelocity = 0.0f;
    float mYSpeed = 0.0f;
    float mYVelocity = 0.0f;
};

class Pawn
{
public:
    Pawn(ResourceLocator& resLoc);
    ~Pawn();

    void Update();

    void Render(AbstractRenderer& rend) const;

private:
    AnimationComponent mAnimation;
    PhysicsComponent mPhysics;
};

inline std::unique_ptr<Pawn> CreateTestPawn(ResourceLocator& resLoc)
{
    return std::make_unique<Pawn>(resLoc);
}
