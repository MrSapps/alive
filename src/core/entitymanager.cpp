#include <cstring>
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

void EntityManager::Serialize(std::ostream& os) const
{
    auto skip = true;
    for (auto const& entity : mEntities)
    {
        if (skip)
        {
            skip = false;
            continue;
        }
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
	mEntities.erase(mEntities.begin() + 1, mEntities.end());
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
            else if(token == '{')
            {
                entity = Create();
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

#if defined(_DEBUG)
bool EntityManager::IsComponentRegistered(const std::string& componentName) const
{
    return mRegisteredComponents.find(componentName) != mRegisteredComponents.end();
}
#endif