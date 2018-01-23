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
    entity->mDestroyed = true;
}

void EntityManager::DestroyEntities()
{
    mEntities.erase(std::remove_if(mEntities.begin(), mEntities.end(), [](auto const& e)
    {
        return e.get()->mDestroyed;
    }), mEntities.end());
}