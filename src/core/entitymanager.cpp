#include <cstring>
#include <algorithm>

#include "core/entitymanager.hpp"

Entity* EntityManager::CreateEntity()
{
    auto entity = std::make_unique<Entity>(this);
    auto entityPtr = entity.get();
    mEntities.emplace_back(std::move(entity));
    return entityPtr;
}

void EntityManager::DestroyEntity(Entity* entity)
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

void EntityManager::ConstructSystem(System& system)
{
	system.mManager = this;
	system.OnLoad();
}

bool EntityManager::IsComponentRegistered(const std::string& componentName) const
{
    return mRegisteredComponents.find(componentName) != mRegisteredComponents.end();
}

void EntityManager::Update()
{
    for (auto& system : mSystems)
    {
        system->Update();
    }
    DestroyEntities();
}

void EntityManager::Serialize(std::ostream& os) const
{
    for (auto const& entity : mEntities)
    {
        os.write("{", 1);
        for (auto const& component : entity->mComponents)
        {
            os.write(component->GetComponentName().c_str(), 1 + component->GetComponentName().size());
            component->Serialize(os);
        }
        os.write("}", 1);
    }
}

void EntityManager::Deserialize(std::istream& is)
{
    mEntities.clear();
    enum class mode_e
    {
        entity_create, component_name
    };
    auto mode = mode_e::entity_create;
    Entity* entity = nullptr;
    std::string componentName;
    while (!is.eof())
    {
        char token;
        is >> token;
        if (mode == mode_e::entity_create)
        {
            if (token == '0')
            {
                break;
            }
            else if (token == '{')
            {
                entity = CreateEntity();
                mode = mode_e::component_name;
            }
        }
        else if (mode == mode_e::component_name)
        {
            if (token == '}')
            {
                entity->ResolveComponentDependencies();
                mode = mode_e::entity_create;
            }
            else if (token == '\0')
            {
                auto componentCreator = mRegisteredComponents.find(componentName);
                if (componentCreator == mRegisteredComponents.end())
                {
                    throw std::logic_error(componentName + std::string { " is not registered" });
                }
                auto component = componentCreator->second();
                auto componentPtr = component.get();
                entity->mComponents.emplace_back(std::move(component));
                componentPtr->Deserialize(is);
                entity->ConstructComponent(*componentPtr);
                componentName.clear();
                mode = mode_e::component_name;
            }
            else
            {
                componentName += token;
            }
        }
    }
}