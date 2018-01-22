#pragma once

#include <map>
#include <memory>
#include <functional>

#include "component.hpp"

class EntityManager;

class Entity final
{
public:
    explicit Entity(EntityManager* manager);

public:
    Entity(Entity&&) = delete;
    Entity(Entity const&) = delete;
    Entity& operator=(Entity const&) = delete;

public:
    template<typename C>
    C* GetComponent();
    template<typename C, typename ...Args>
    C* AddComponent(Args&& ...args);
    template<typename C>
    void RemoveComponent();

public:
    template<typename C>
    bool HasComponent() const;
    template<typename C1, typename C2, typename ...C>
    bool HasComponent() const;
    template<typename C>
    bool HasAnyComponent() const;
    template<typename C1, typename C2, typename ...C>
    bool HasAnyComponent() const;

public:
    template<typename ...C>
    bool Any(typename std::common_type<std::function<void(C* ...)>>::type view);
    template<typename ...C>
    bool With(typename std::common_type<std::function<void(C* ...)>>::type view);

public:
    void Destroy();

private:
    EntityManager* mManager;
    std::map<const char*, std::unique_ptr<Component>> mComponents;
};

template<typename C, typename ...Args>
C* Entity::AddComponent(Args&& ...args)
{
    auto component = std::make_unique<C>(std::forward<Args>(args)...);
    auto componentPtr = component.get();
    componentPtr->mEntity = this;
    mComponents[C::ComponentName] = std::move(component);
    return componentPtr;
}

template<typename C>
C* Entity::GetComponent()
{
    auto found = mComponents.find(C::ComponentName);
    if (found != mComponents.end())
    {
        return static_cast<C*>(found->second.get());
    }
    return nullptr;
}

template<typename C>
void Entity::RemoveComponent()
{
    auto found = mComponents.find(C::ComponentName);
    if (found != mComponents.end())
    {
        mComponents.erase(found);
    }
}

template<typename C>
bool Entity::HasComponent() const
{
    return mComponents.find(C::ComponentName) != mComponents.end();
}

template<typename C1, typename C2, typename ...C>
bool Entity::HasComponent() const
{
    return HasComponent<C1>() && HasComponent<C2, C...>();
}

template<typename C>
bool Entity::HasAnyComponent() const
{
    return mComponents.find(C::ComponentName) != mComponents.end();
}

template<typename C1, typename C2, typename ...C>
bool Entity::HasAnyComponent() const
{
    if (HasComponent<C1>()) {
        return true;
    }
    return HasComponent<C2, C...>();
}

template<typename ...C>
bool Entity::Any(typename std::common_type<std::function<void(C* ...)>>::type view)
{
    if (HasAnyComponent<C...>())
    {
        view(GetComponent<C>()...);
        return true;
    }
    return false;
}

template<typename ...C>
bool Entity::With(typename std::common_type<std::function<void(C* ...)>>::type view)
{
    if (HasComponent<C...>())
    {
        view(GetComponent<C>()...);
        return true;
    }
    return false;
}