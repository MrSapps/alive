#include "core/entity.hpp"
#include "core/entitymanager.hpp"

Entity::Entity(EntityManager* manager) : mManager(manager)
{

}

Entity::Entity(EntityManager* manager, PointerSize index, PointerSize version) : mManager(manager), mIndex(index), mVersion(version)
{

}

bool Entity::IsValid() const
{
    return mManager != nullptr && mManager->IsEntityPointerValid(*this);
}

Entity::operator bool() const
{
    return IsValid();
}

void Entity::Destroy()
{
    mManager->DestroyEntity(*this);
}

void Entity::ResolveComponentDependencies()
{
    mManager->EntityResolveComponentDependencies(*this);
}

EntityManager* Entity::GetManager()
{
    return mManager;
}

bool operator==(const Entity& a, const Entity& b)
{
    return a.mIndex == b.mIndex && a.mVersion == b.mVersion;
}