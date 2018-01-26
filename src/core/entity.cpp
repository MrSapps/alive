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

void Entity::ResolveComponentDependencies()
{
    for (auto &component : mComponents)
    {
        component->OnResolveDependencies();
    }
}

void Entity::Destroy()
{
    mManager->Destroy(this);
}

bool Entity::IsDestroyed() const
{
    return mDestroyed;
}

void Entity::ConstructComponent(Component& component)
{
    component.mEntity = this;
    component.OnLoad();
}

#if defined(_DEBUG)
void Entity::AssertComponentRegistered(const std::string& componentName) const
{
    if (!mManager->IsComponentRegistered(componentName))
    {
        throw std::logic_error(std::string{ "Entity::AssertComponentRegistered: Component " } + componentName + std::string{ " not registered in EntityManager" });
    }
}
#endif