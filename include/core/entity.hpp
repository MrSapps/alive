#pragma once

#include <memory>
#include <vector>

#include "resourcemapper.hpp"

enum class ComponentIdentifier {
	Animation,
	Physics
};

class Component {
public:
	virtual ~Component() {};
public:
	virtual void Update() {};
	virtual void Render(AbstractRenderer&) const {};
};

class AnimationComponent : public Component
{
public:
    AnimationComponent();
public:
    void Update() final;
    void Load(ResourceLocator& resLoc, const char* animationName);
    void Render(AbstractRenderer& rend) const final;
private:
    std::unique_ptr<Animation> mAnimation;
};

class PhysicsComponent : public Component
{
public:
    void Update() final;
public:
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
public:
	void Init();
    void Update();
    void Render(AbstractRenderer& rend) const;

public:
	template<typename T>
	void AddComponent(ComponentIdentifier id); // TODO: component identifier

public:
	std::vector<std::unique_ptr<Component>> mComponents;
	AnimationComponent *mAnimationComponent;

private:
	ResourceLocator &mResourceLocator;
};

inline std::unique_ptr<Pawn> CreateTestPawn(ResourceLocator& resLoc)
{
    return std::make_unique<Pawn>(resLoc);
}

template<typename T>
inline void Pawn::AddComponent(ComponentIdentifier)
{
	mComponents.emplace_back(std::move(std::make_unique<T>())); // TODO: forward arguments?
}