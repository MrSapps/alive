#pragma once

#include <memory>
#include <vector>

#include "core/component.hpp"
#include "resourcemapper.hpp"

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

class AbeEntity final : public Entity
{
public:
    explicit AbeEntity(ResourceLocator& resLoc);
};

class SligEntity final : public Entity
{
public:
    explicit SligEntity(ResourceLocator& resLoc);
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