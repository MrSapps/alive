#if defined(_DEBUG)
#   include <string>
#   include <cassert>
#   include <stdexcept>
#endif

#include "core/entity.hpp"
#include "core/entitymanager.hpp"

Entity::Entity(EntityManager* manager) : mManager(manager)
{

}

EntityManager* Entity::GetManager()
{
    return mManager;
}

void Entity::Destroy()
{
    mManager->Destroy(this);
}

bool Entity::IsDestroyed() const
{
    return mDestroyed;
}

#if defined(_DEBUG)
void Entity::AssertComponentRegistered(const char* componentName) const
{
    if (!mManager->IsComponentRegistered(componentName))
    {
        throw std::logic_error(std::string{ "The component " } + componentName + std::string{ " is not registered" });
    }
}
#endif