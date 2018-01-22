#include "core/entity.hpp"
#include "core/entitymanager.hpp"

Entity::Entity(EntityManager* manager) : mManager(manager)
{

}

void Entity::Destroy()
{
    mManager->Destroy(this);
}