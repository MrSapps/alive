#include <map>
#include <memory>
#include <vector>
#include <string>
#include <functional>

#include "entity.hpp"

class EntityManager final
{
public:
    Entity* Create();
    template<typename ...C>
    Entity* CreateWith();

private:
    template<typename C>
    void DispatchCreateWith(Entity *entity);
    template<typename C1, typename C2, typename ...C>
    void DispatchCreateWith(Entity *entity);

public:
    void Destroy(Entity* entity);
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
    template<typename C>
    void RegisterComponent();
#if defined(_DEBUG)
    bool IsComponentRegistered(const std::string& componentName) const;
#endif

public:
    void Serialize(std::ostream& os) const;
    void Deserialize(std::istream& is);

private:
    std::vector<std::unique_ptr<Entity>> mEntities;
    std::map<std::string, std::function<std::unique_ptr<Component>()>> mRegisteredComponents;
};

template<typename ...C>
Entity* EntityManager::CreateWith()
{
    auto entity = Create();
    DispatchCreateWith<C...>(entity);
    entity->ResolveComponentDependencies();
    return entity;
}

template<typename C>
void EntityManager::DispatchCreateWith(Entity *entity)
{
    entity->AddComponent<C>();
}

template<typename C1, typename C2, typename ...C>
void EntityManager::DispatchCreateWith(Entity *entity)
{
    DispatchCreateWith<C1>(entity);
    DispatchCreateWith<C2, C...>(entity);
};

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