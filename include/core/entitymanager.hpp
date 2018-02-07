#pragma once

#include <memory>
#include <vector>
#include <string>
#include <ostream>
#include <istream>
#include <stdexcept>
#include <algorithm>
#include <functional>
#include <type_traits>
#include <unordered_map>

#include "system.hpp"
#include "entity.hpp"
#include "component.hpp"

class EntityManager final
{
public:
    friend Entity;

public:
    Entity CreateEntity();
    template<typename ...C>
    Entity CreateEntityWith();

private:
    template<typename C>
    void CreateEntityWith(Entity& entityPointer);
    template<typename C1, typename C2, typename ...C>
    void CreateEntityWith(Entity& entityPointer);

private:
    void DestroyEntity(Entity& entityPointer);

public:
    template<typename S, typename ...Args>
    S* AddSystem(Args&& ...args);
    template<typename S>
    S* GetSystem();
    template<typename S>
    const S* GetSystem() const;
    template<typename S>
    void RemoveSystem();
    template<typename S>
    bool HasSystem();

public:
    template<typename C>
    void RegisterComponent();
#if defined(_DEBUG)
    bool IsComponentRegistered(const std::string& componentName) const;
    void AssertComponentRegistered(const std::string& componentName) const;
#endif

public:
    template<typename ...C>
    void Any(typename std::common_type<std::function<void(Entity, C* ...)>>::type view);
    template<typename ...C>
    std::vector<Entity> Any();
    template<typename ...C>
    void With(typename std::common_type<std::function<void(Entity, C* ...)>>::type view);
    template<typename ...C>
    std::vector<Entity> With();

public:
    void Serialize(std::ostream& os) const;
    void Deserialize(std::istream& is);

private:
    bool IsEntityPointerValid(const Entity& entityPointer) const;
    void AssertEntityPointerValid(const Entity& entityPointer) const;

public:
    template<bool is_const>
    class EntityComponentContainerIterator final
    {
    public:
        friend EntityManager;

    public:
        using EntityType = std::conditional_t<is_const, const Entity, Entity>;
        using ManagerType = std::conditional_t<is_const, const EntityManager, EntityManager>;

    public:
        EntityComponentContainerIterator(ManagerType& manager, Entity::PointerSize position) : mManager(manager), mIndex(position), mVersion(0)
        {
            IterateToNextValidEntity();
        }

    public:
        EntityType operator*()
        {
            return EntityType(const_cast<EntityManager*>(&mManager), mIndex, mVersion);
        }
        bool operator!=(const EntityComponentContainerIterator& other)
        {
            return mIndex != other.mIndex;
        }
        const EntityComponentContainerIterator& operator++()
        {
            {
                mIndex += 1;
                IterateToNextValidEntity();
                return *this;
            }
        }

    private:
        void IterateToNextValidEntity()
        {
            while (mIndex < mManager.mNextIndex && std::find(mManager.mFreeIndexes.begin(), mManager.mFreeIndexes.end(), mIndex) != mManager.mFreeIndexes.end())
            {
                mIndex += 1;
            }
            if (mIndex < mManager.mNextIndex)
            {
                mVersion = mManager.mVersions[mIndex];
            }
        }

    private:
        ManagerType& mManager;
        Entity::PointerSize mIndex;
        Entity::PointerSize mVersion;
    };

public:
    using Iterator = EntityComponentContainerIterator<false>;
    using ConstIterator = EntityComponentContainerIterator<true>;

public:
    Iterator begin();
    Iterator end();
    ConstIterator begin() const;
    ConstIterator end() const;

public:
    void Clear();
    std::size_t Size() const;

private:
    template<typename C>
    C* EntityGetComponent(const Entity& entityPointer);
    template<typename C>
    const C* EntityGetComponent(const Entity& entityPointer) const;
    template<typename C>
    C* EntityAddComponent(const Entity& entityPointer);
    template<typename C>
    void EntityRemoveComponent(const Entity& entityPointer);

private:
    void EntityConstructComponent(Component* component, const Entity& entityPointer);
    void EntityResolveComponentDependencies(const Entity& entityPointer);

private:
    template<typename C>
    bool EntityHasComponent(const Entity& entityPointer) const;
    template<typename C1, typename C2, typename ...C>
    bool EntityHasComponent(const Entity& entityPointer) const;
    template<typename C>
    bool EntityHasAnyComponent(const Entity& entityPointer) const;
    template<typename C1, typename C2, typename ...C>
    bool EntityHasAnyComponent(const Entity& entityPointer) const;

private:
    template<typename ...C>
    bool EntityAny(const Entity& entityPointer, typename std::common_type<std::function<void(C* ...)>>::type view);
    template<typename ...C>
    bool EntityWith(const Entity& entityPointer, typename std::common_type<std::function<void(C* ...)>>::type view);

private:
    Entity::PointerSize mNextIndex = {};
    std::vector<Entity::PointerSize> mVersions = {};
    std::vector<Entity::PointerSize> mFreeIndexes = {};
    std::vector<std::vector<std::unique_ptr<Component>>> mEntityComponents = {};

private:
    std::vector<std::unique_ptr<System>> mSystems = {};
    std::unordered_map<std::string, std::function<std::unique_ptr<Component>()>> mRegisteredComponents = {};
};

template<typename C>
C* Entity::GetComponent()
{
    return mManager->EntityGetComponent<C>(*this);
}

template<typename C>
const C* Entity::GetComponent() const
{
    return mManager->EntityGetComponent<C>(*this);
}

template<typename C>
C* Entity::AddComponent()
{
    return mManager->EntityAddComponent<C>(*this);
}

template<typename C>
void Entity::RemoveComponent()
{
    mManager->EntityRemoveComponent<C>(*this);
}

template<typename C>
bool Entity::HasComponent() const
{
    return mManager->EntityHasComponent<C>(*this);
}

template<typename C1, typename C2, typename ...C>
bool Entity::HasComponent() const
{
    return mManager->EntityHasComponent<C1, C2, C...>(*this);
}

template<typename C>
bool Entity::HasAnyComponent() const
{
    return mManager->EntityHasAnyComponent<C>(*this);
}

template<typename C1, typename C2, typename ...C>
bool Entity::HasAnyComponent() const
{
    return mManager->EntityHasAnyComponent<C1, C2, C...>(*this);
}

template<typename... C>
bool Entity::Any(typename std::common_type<std::function<void(C* ...)>>::type view)
{
    return mManager->EntityAny(*this, view);
}

template<typename... C>
bool Entity::With(typename std::common_type<std::function<void(C* ...)>>::type view)
{
    return mManager->EntityWith(*this, view);
}

template<typename... C>
Entity EntityManager::CreateEntityWith()
{
    auto entityPointer = CreateEntity();
    CreateEntityWith<C...>(entityPointer);
    EntityResolveComponentDependencies(entityPointer);
    return entityPointer;
}

template<typename C>
void EntityManager::CreateEntityWith(Entity& entityPointer)
{
    entityPointer.AddComponent<C>();
}

template<typename C1, typename C2, typename ...C>
void EntityManager::CreateEntityWith(Entity& entityPointer)
{
    CreateEntityWith<C1>(entityPointer);
    CreateEntityWith<C2, C...>(entityPointer);
}

template<typename S, typename... Args>
S* EntityManager::AddSystem(Args&& ... args)
{
    if (HasSystem<S>())
    {
        throw std::logic_error(std::string{ "EntityManager::AddSystem: System " } + S::SystemName + std::string{ " already exists" });
    }
    auto system = std::make_unique<S>(std::forward<Args>(args)...);
    auto systemPtr = system.get();
    mSystems.emplace_back(std::move(system));
    systemPtr->mManager = this;
    return systemPtr;
}

template<typename S>
S* EntityManager::GetSystem()
{
    return const_cast<S*>(static_cast<const EntityManager*>(this)->GetSystem<S>());
}

template<typename S>
const S* EntityManager::GetSystem() const
{
    auto found = std::find_if(mSystems.begin(), mSystems.end(), [](const auto& s)
    {
        return s->GetSystemName() == S::SystemName;
    });
    if (found != mSystems.end())
    {
        return static_cast<S*>(found->get());
    }
    return nullptr;
}

template<typename S>
void EntityManager::RemoveSystem()
{
    if (!HasSystem<S>())
    {
        throw std::logic_error(std::string{ "EntityManager::RemoveSystem: System " } + S::SystemName + std::string{ " not found" });
    }
    auto found = std::find_if(mSystems.begin(), mSystems.end(), [](const auto& s)
    {
        return s->GetSystemName() == S::SystemName;
    });
    if (found != mSystems.end())
    {
        mSystems.erase(found);
    }
}

template<typename S>
bool EntityManager::HasSystem()
{
    return GetSystem<S>() != nullptr;
}

template<typename C>
void EntityManager::RegisterComponent()
{
    mRegisteredComponents[C::ComponentName] = []()
    {
        return std::make_unique<C>();
    };
}

template<typename C>
C* EntityManager::EntityGetComponent(const Entity& entityPointer)
{
    AssertEntityPointerValid(entityPointer);
    return const_cast<C*>(static_cast<const EntityManager*>(this)->EntityGetComponent<C>(entityPointer));
}

template<typename C>
const C* EntityManager::EntityGetComponent(const Entity& entityPointer) const
{
    AssertEntityPointerValid(entityPointer);
    auto& components = mEntityComponents[entityPointer.mIndex];
    auto found = std::find_if(components.begin(), components.end(), [](const auto& c)
    {
        return c->GetComponentName() == C::ComponentName;
    });
    if (found != components.end())
    {
        return static_cast<C*>(found->get());
    }
    return nullptr;
}

template<typename C>
C* EntityManager::EntityAddComponent(const Entity& entityPointer)
{
    AssertEntityPointerValid(entityPointer);
#if defined(_DEBUG)
    AssertComponentRegistered(C::ComponentName);
#endif
    if (EntityHasComponent<C>(entityPointer))
    {
        throw std::logic_error(std::string{ "Entity::AddComponent: Component " } + C::ComponentName + std::string{ " already exists" });
    }
    auto component = std::make_unique<C>();
    auto componentPtr = component.get();
    mEntityComponents[entityPointer.mIndex].emplace_back(std::move(component));
    EntityConstructComponent(componentPtr, entityPointer);
    return componentPtr;

}

template<typename C>
void EntityManager::EntityRemoveComponent(const Entity& entityPointer)
{
    AssertEntityPointerValid(entityPointer);
#if defined(_DEBUG)
    AssertComponentRegistered(C::ComponentName);
#endif
    auto& components = mEntityComponents[entityPointer.mIndex];
    if (!EntityHasComponent<C>(entityPointer))
    {
        throw std::logic_error(std::string{ "Entity::RemoveComponent: Component " } + C::ComponentName + std::string{ " not found" });
    }
    auto found = std::find_if(components.begin(), components.end(), [](const auto& c)
    {
        return C::ComponentName == c->GetComponentName();
    });
    if (found != components.end())
    {
        components.erase(found);
    }
}

template<typename C>
bool EntityManager::EntityHasComponent(const Entity& entityPointer) const
{
    AssertEntityPointerValid(entityPointer);
    return EntityGetComponent<C>(entityPointer) != nullptr;
}

template<typename C1, typename C2, typename ...C>
bool EntityManager::EntityHasComponent(const Entity& entityPointer) const
{
    AssertEntityPointerValid(entityPointer);
    return EntityHasComponent<C1>(entityPointer) && EntityHasComponent<C2, C...>(entityPointer);
}

template<typename C>
bool EntityManager::EntityHasAnyComponent(const Entity& entityPointer) const
{
    AssertEntityPointerValid(entityPointer);
    return EntityGetComponent<C>(entityPointer) != nullptr;
}

template<typename C1, typename C2, typename ...C>
bool EntityManager::EntityHasAnyComponent(const Entity& entityPointer) const
{
    AssertEntityPointerValid(entityPointer);
    if (EntityHasComponent<C1>(entityPointer))
    {
        return true;
    }
    return EntityHasComponent<C2, C...>(entityPointer);
}

template<typename... C>
bool EntityManager::EntityAny(const Entity& entityPointer, typename std::common_type<std::function<void(C* ...)>>::type view)
{
    AssertEntityPointerValid(entityPointer);
    if (EntityHasAnyComponent<C...>(entityPointer))
    {
        view(EntityGetComponent<C>(entityPointer)...);
        return true;
    }
    return false;
}

template<typename... C>
bool EntityManager::EntityWith(const Entity& entityPointer, typename std::common_type<std::function<void(C* ...)>>::type view)
{
    AssertEntityPointerValid(entityPointer);
    if (EntityHasComponent<C...>(entityPointer))
    {
        view(EntityGetComponent<C>(entityPointer)...);
        return true;
    }
    return false;
}

template<typename... C>
void EntityManager::Any(typename std::common_type<std::function<void(Entity, C* ...)>>::type view)
{
    for (auto entityPointer : *this)
    {
        if (entityPointer.HasAnyComponent<C...>())
        {
            view(entityPointer, entityPointer.GetComponent<C>()...);
        }
    }
}

template<typename... C>
std::vector<Entity> EntityManager::Any()
{
    std::vector<Entity> entityPointers;
    for (auto entityPointer : *this)
    {
        if (entityPointer.HasAnyComponent<C...>())
        {
            entityPointers.emplace_back(entityPointer);
        }
    }
    return entityPointers;
}

template<typename... C>
void EntityManager::With(typename std::common_type<std::function<void(Entity, C* ...)>>::type view)
{
    for (auto entityPointer : *this)
    {
        if (entityPointer.HasComponent<C...>())
        {
            view(entityPointer, entityPointer.GetComponent<C>()...);
        }
    }
}

template<typename... C>
std::vector<Entity> EntityManager::With()
{
    std::vector<Entity> entityPointers;
    for (auto entityPointer : *this)
    {
        if (entityPointer.HasComponent<C...>())
        {
            entityPointers.emplace_back(entityPointer);
        }
    }
    return entityPointers;
}