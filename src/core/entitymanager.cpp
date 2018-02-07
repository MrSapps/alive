#include <cstring>
#include <sstream>

#include "core/entitymanager.hpp"

Entity EntityManager::CreateEntity()
{
    Entity::PointerSize index;
    Entity::PointerSize version;
    if (mFreeIndexes.empty())
    {
        index = mNextIndex++;
        mVersions.resize(index + 1);
        mEntityComponents.resize(index + 1);
        version = mVersions[index] = 1;
    }
    else
    {
        index = mFreeIndexes.back();
        version = mVersions[index];
        mFreeIndexes.pop_back();
    }
    return { this, index, version };
}

void EntityManager::DestroyEntity(Entity& entityPointer)
{
    AssertEntityPointerValid(entityPointer);
    mVersions[entityPointer.mIndex] += 1;
    mEntityComponents[entityPointer.mIndex].clear();
    mFreeIndexes.push_back(entityPointer.mIndex);
}

void EntityManager::Serialize(std::ostream& os) const
{
    for (auto entityPointer : *this)
    {
        os << '{';
        os.write(reinterpret_cast<char*>(&entityPointer.mIndex), sizeof(entityPointer.mIndex));
        os.write(reinterpret_cast<char*>(&entityPointer.mVersion), sizeof(entityPointer.mVersion));
        for (const auto& component : mEntityComponents[entityPointer.mIndex])
        {
            os.write(component->GetComponentName().c_str(), 1 + component->GetComponentName().size());
            component->Serialize(os);
        }
        os << '}';
    }
}

void EntityManager::Deserialize(std::istream& is)
{
    Clear();
    enum class ParsingState
    {
        eEntity,
        eComponentName,
    };
    auto state = ParsingState::eEntity;
    std::unique_ptr<Entity> entityPointer;
    std::string componentName;
    while (!is.eof())
    {
        char token;
        is >> token;
        if (state == ParsingState::eEntity)
        {
            if (token == '0')
            {
                break;
            }
            else if (token == '{')
            {
                entityPointer = std::make_unique<Entity>(this);
                is.read(reinterpret_cast<char*>(&entityPointer->mIndex), sizeof(entityPointer->mIndex));
                is.read(reinterpret_cast<char*>(&entityPointer->mVersion), sizeof(entityPointer->mVersion));
                for (auto i = mNextIndex; i < entityPointer->mIndex; i++)
                {
                    mFreeIndexes.emplace_back(i);
                }
                mNextIndex = static_cast<Entity::PointerSize>(entityPointer->mIndex + 1);
                mVersions.resize(mNextIndex);
                mEntityComponents.resize(mNextIndex);
                mVersions[entityPointer->mIndex] = entityPointer->mVersion;
                state = ParsingState::eComponentName;
            }
        }
        else if (state == ParsingState::eComponentName)
        {
            if (token == '}')
            {
                entityPointer->ResolveComponentDependencies();
                state = ParsingState::eEntity;
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
                mEntityComponents[entityPointer->mIndex].emplace_back(std::move(component));
                componentPtr->Deserialize(is);
                EntityConstructComponent(componentPtr, *(entityPointer.get()));
                componentName.clear();
                state = ParsingState::eComponentName;
            }
            else
            {
                componentName += token;
            }
        }
    }
}

bool EntityManager::IsEntityPointerValid(const Entity& entityPointer) const
{
    return entityPointer.mIndex < mVersions.size() && mVersions[entityPointer.mIndex] == entityPointer.mVersion;
}

void EntityManager::AssertEntityPointerValid(const Entity& entityPointer) const
{
    if (!IsEntityPointerValid(entityPointer))
    {
        std::stringstream errorFormat;
        errorFormat << "Entity invalid: " << entityPointer.mIndex << "(" << entityPointer.mIndex << ")";
        throw std::logic_error(errorFormat.str());
    }
}

void EntityManager::EntityConstructComponent(Component* component, const Entity& entityPointer)
{
    AssertEntityPointerValid(entityPointer);
    component->mEntity = entityPointer;
    component->OnLoad();
}

void EntityManager::EntityResolveComponentDependencies(const Entity& entityPointer)
{
    AssertEntityPointerValid(entityPointer);
    auto& components = mEntityComponents[entityPointer.mIndex];
    for (auto& component : components)
    {
        component->OnResolveDependencies();
    }
}

#if defined(_DEBUG)
bool EntityManager::IsComponentRegistered(const std::string& componentName) const
{
    return mRegisteredComponents.find(componentName) != mRegisteredComponents.end();
}

void EntityManager::AssertComponentRegistered(const std::string& componentName) const
{
    if (!IsComponentRegistered(componentName))
    {
        throw std::logic_error(std::string{ "EntityManager::AssertComponentRegistered: Component " } + componentName + std::string{ " not registered" });
    }
}
#endif

EntityManager::Iterator EntityManager::begin()
{
    return { *this, 0 };
}

EntityManager::Iterator EntityManager::end()
{
    return { *this, static_cast<Entity::PointerSize>(mEntityComponents.size()) };
}

EntityManager::ConstIterator EntityManager::begin() const
{
    return { *this, 0 };
}

EntityManager::ConstIterator EntityManager::end() const
{
    return { *this, static_cast<Entity::PointerSize>(mEntityComponents.size()) };
}

void EntityManager::Clear()
{
    mNextIndex = 0;
    mVersions.clear();
    mFreeIndexes.clear();
    mEntityComponents.clear();
}

std::size_t EntityManager::Size() const
{
    return mEntityComponents.size() - mFreeIndexes.size();
}