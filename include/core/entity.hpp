#pragma once

#include <memory>
#include <vector>
#include <string>
#include <algorithm>
#include <functional>

#include "component.hpp"

class EntityManager;

class Entity final
{
public:
    friend EntityManager;

public:
    explicit Entity(EntityManager* manager);

public:
    Entity(Entity&&) = delete;
    Entity(Entity const&) = delete;
    Entity& operator=(Entity const&) = delete;

public:
    EntityManager* GetManager();

public:
    template<typename C>
    C* GetComponent();
    template<typename C>
    const C* GetComponent() const;
    template<typename C>
    C* AddComponent();
    template<typename C>
    void RemoveComponent();
    template<typename C>
    bool HasComponent() const;
    template<typename C1, typename C2, typename ...C>
    bool HasComponent() const;
    template<typename C>
    bool HasAnyComponent() const;
    template<typename C1, typename C2, typename ...C>
    bool HasAnyComponent() const;

public:
    void ResolveComponentDependencies();

public:
    template<typename ...C>
    bool Any(typename std::common_type<std::function<void(C* ...)>>::type view);
    template<typename ...C>
    bool With(typename std::common_type<std::function<void(C* ...)>>::type view);

public:
    void Destroy();
    bool IsDestroyed() const;

private:
    void ConstructComponent(Component& component);

private:
#if defined(_DEBUG)
    void AssertComponentRegistered(const std::string& componentName) const;
#endif

private:
    bool mDestroyed = false;
    EntityManager* mManager = nullptr;
    std::vector<std::unique_ptr<Component>> mComponents;
};

template<typename C>
C* Entity::GetComponent()
{
    return const_cast<C*>(static_cast<const Entity*>(this)->GetComponent<C>());
}

template<typename C>
const C* Entity::GetComponent() const
{
    auto found = std::find_if(mComponents.begin(), mComponents.end(), [](const auto& c)
    {
        return c->GetComponentName() == C::ComponentName;
    });
    if (found != mComponents.end())
    {
        return static_cast<C*>(found->get());
    }
    return nullptr;
}

template<typename C>
C* Entity::AddComponent()
{
#if defined(_DEBUG)
    AssertComponentRegistered(C::ComponentName);
#endif
    if (HasComponent<C>())
    {
        throw std::logic_error(std::string{ "Entity::AddComponent: Component " } + C::ComponentName + std::string{ " already exists" });
    }
    auto component = std::make_unique<C>();
    auto componentPtr = component.get();
    mComponents.emplace_back(std::move(component));
    ConstructComponent(*componentPtr);
    return componentPtr;
}

template<typename C>
void Entity::RemoveComponent()
{
#if defined(_DEBUG)
    AssertComponentRegistered(C::ComponentName);
#endif
    if (!HasComponent<C>())
    {
        throw std::logic_error(std::string{ "Entity::RemoveComponent: Component " } + C::ComponentName + std::string{ " not found" });
    }
    auto found = std::find_if(mComponents.begin(), mComponents.end(), [](const auto& c)
    {
        return C::ComponentName == c->GetComponentName();
    });
    if (found != mComponents.end())
    {
        mComponents.erase(found);
    }
}

template<typename C>
bool Entity::HasComponent() const
{
    return GetComponent<C>() != nullptr;
}

template<typename C1, typename C2, typename ...C>
bool Entity::HasComponent() const
{
    return HasComponent<C1>() && HasComponent<C2, C...>();
}

template<typename C>
bool Entity::HasAnyComponent() const
{
    return GetComponent<C>() != nullptr;
}

template<typename C1, typename C2, typename ...C>
bool Entity::HasAnyComponent() const
{
    if (HasComponent<C1>())
    {
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