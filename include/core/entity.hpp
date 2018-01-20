#pragma once

#include <memory>
#include <vector>

#include "resourcemapper.hpp"

enum class ComponentIdentifier
{
    Animation,
    Physics
};

class Entity;

class Component
{
public:
    using UPtr = std::unique_ptr<Component>;
public:
    virtual ~Component() = default;
public:
    virtual void Update();
    virtual void Render(AbstractRenderer&) const;
public:
    void SetEntity(Entity*);
    void SetId(ComponentIdentifier);
    ComponentIdentifier GetId() const;
protected:
    Entity* mEntity;
    ComponentIdentifier mId;
};

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

class Entity
{
public:
    using UPtr = std::unique_ptr<Entity>;
public:
    Entity() = default;
    virtual ~Entity() = default;
public:
    void Update();
    void Render(AbstractRenderer&) const;
public:
    void AddChild(UPtr child);
public:
    template<typename T>
    T* AddComponent(ComponentIdentifier id);
    template<typename T>
    T* GetComponent(ComponentIdentifier id);
private:
    std::vector<UPtr> mChildren;
    std::vector<Component::UPtr> mComponents;
};

class AbeEntity : public Entity
{
public:
    explicit AbeEntity(ResourceLocator& resLoc);
public:
    PhysicsComponent* mPhysicsComponent;
    AnimationComponent* mAnimationComponent;
};

class SligEntity : public Entity
{
public:
    explicit SligEntity(ResourceLocator& resLoc);
public:
    PhysicsComponent* mPhysicsComponent;
    AnimationComponent* mAnimationComponent;
};

template<typename T>
inline T* Entity::AddComponent(ComponentIdentifier id)
{
    auto comp = std::make_unique<T>(); // TODO: forward arguments?
    auto compPtr = comp.get();
    compPtr->SetEntity(this);
    compPtr->SetId(id);
    mComponents.emplace_back(std::move(comp));
    return compPtr;
}

template<typename T>
inline T* Entity::GetComponent(ComponentIdentifier id)
{
    auto found = std::find_if(mComponents.begin(), mComponents.end(), [id](auto const& comp)
    {
        return comp->GetId() == id;
    });
    if (found != mComponents.end())
    {
        return static_cast<T*>(found->get());
    }
    return nullptr;
}