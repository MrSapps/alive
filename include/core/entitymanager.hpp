#include <map>
#include <memory>
#include <vector>
#include <string>
#include <utility>
#include <functional>

#include "entity.hpp"
#include "system.hpp"

class EntityManager final
{
public:
    Entity* CreateEntity();
    template<typename ...C>
    Entity* CreateEntityWith();

private:
    template<typename C>
    void CreateEntityWith(Entity* entity);
    template<typename C1, typename C2, typename ...C>
    void CreateEntityWith(Entity* entity);

public:
    void DestroyEntity(Entity* entity);
    void DestroyEntities();

public:
    template<typename ...C>
    void Any(typename std::common_type<std::function<void(Entity*, C* ...)>>::type view);
    template<typename ...C>
    std::vector<Entity*> Any();
    template<typename ...C>
    void With(typename std::common_type<std::function<void(Entity*, C* ...)>>::type view);
    template<typename ...C>
    std::vector<Entity*> With();

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

private:
    void ConstructSystem(System& system);

public:
    template<typename C>
    void RegisterComponent();
    bool IsComponentRegistered(const std::string& componentName) const;

public:
    void Update();

public:
    void Serialize(std::ostream& os) const;
    void Deserialize(std::istream& is);

public:
    std::vector<std::unique_ptr<Entity>> mEntities; // TODO private

private:
    std::vector<std::unique_ptr<System>> mSystems;
    std::map<std::string, std::function<std::unique_ptr<Component>()>> mRegisteredComponents;
};

template<typename ...C>
Entity* EntityManager::CreateEntityWith()
{
    auto entity = CreateEntity();
    CreateEntityWith<C...>(entity);
    entity->ResolveComponentDependencies();
    return entity;
}

template<typename C>
void EntityManager::CreateEntityWith(Entity* entity)
{
    entity->AddComponent<C>();
}

template<typename C1, typename C2, typename ...C>
void EntityManager::CreateEntityWith(Entity* entity)
{
    CreateEntityWith<C1>(entity);
    CreateEntityWith<C2, C...>(entity);
}

template<typename... C>
void EntityManager::Any(typename std::common_type<std::function<void(Entity*, C* ...)>>::type view)
{
    for (auto& entity : mEntities)
    {
        if (entity->HasAnyComponent<C...>())
        {
            view(entity.get(), entity->GetComponent<C>()...);
        }
    }
}

template<typename... C>
std::vector<Entity*> EntityManager::Any()
{
    std::vector<Entity*> entities;
    for (auto& entity : mEntities)
    {
        if (entity->HasAnyComponent<C...>())
        {
            entities.emplace_back(entity.get());
        }
    }
    return entities;
}

template<typename ...C>
void EntityManager::With(typename std::common_type<std::function<void(Entity*, C* ...)>>::type view)
{
    for (auto& entity : mEntities)
    {
        if (entity->HasComponent<C...>())
        {
            view(entity.get(), entity->GetComponent<C>()...);
        }
    }
}

template<typename ...C>
std::vector<Entity*> EntityManager::With()
{
    std::vector<Entity*> entities;
    for (auto& entity : mEntities)
    {
        if (entity->HasComponent<C...>())
        {
            entities.emplace_back(entity.get());
        }
    }
    return entities;
}

template<typename C>
void EntityManager::RegisterComponent()
{
    mRegisteredComponents[C::ComponentName] = []()
    {
        return std::make_unique<C>();
    };
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
    ConstructSystem(*systemPtr);
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