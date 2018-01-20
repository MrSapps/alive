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
public:
    void SetId(ComponentIdentifier);
    ComponentIdentifier GetId() const;
private:
    ComponentIdentifier _id;
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

public:
    void Update();
    void Render(AbstractRenderer& rend) const;

public:
	template<typename T>
	T* AddComponent(ComponentIdentifier id);

public:
	PhysicsComponent *mPhysicsComponent;
	AnimationComponent *mAnimationComponent;

private:
	std::vector<std::unique_ptr<Component>> mComponents;

private:
	ResourceLocator &mResourceLocator;
};

inline std::unique_ptr<Pawn> CreateTestPawn(ResourceLocator& resLoc)
{
    return std::make_unique<Pawn>(resLoc);
}

template<typename T>
inline T* Pawn::AddComponent(ComponentIdentifier id)
{
    auto comp = std::make_unique<T>();
    auto compPtr = comp.get();
    compPtr->SetId(id);
	mComponents.emplace_back(std::move(comp)); // TODO: forward arguments?
	return compPtr;
}