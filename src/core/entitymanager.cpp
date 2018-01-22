#include <algorithm>
#include "core/entitymanager.hpp"

Entity* EntityManager::Create()
{
    auto entity = std::make_unique<Entity>(this);
    auto entityPtr = entity.get();
    mEntities.emplace_back(std::move(entity));
    return entityPtr;
}

void EntityManager::Destroy(Entity* entity)
{
    auto found = std::find_if(mEntities.begin(), mEntities.end(), [&entity](auto const& e)
    {
        return e.get() == entity;
    });
    if (found != mEntities.end())
    {
        mEntities.erase(found);
    }
}