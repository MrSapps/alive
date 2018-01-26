#pragma once

#include <ostream>
#include <istream>

#define DEFINE_COMPONENT(NAME) constexpr const char* NAME::ComponentName; std::string NAME::GetComponentName() const { return NAME::ComponentName; }
#define DECLARE_COMPONENT(NAME) static constexpr const char* ComponentName{#NAME}; virtual std::string GetComponentName() const

class Entity;
class EntityManager;

class Component
{
public:
    DECLARE_COMPONENT(Component);

public:
    friend Entity;
    friend EntityManager;

public:
    virtual ~Component() = 0;

protected:
    virtual void OnLoad();
    virtual void OnResolveDependencies();

protected:
    virtual void Serialize(std::ostream& os) const;
    virtual void Deserialize(std::istream& is);

protected:
    Entity* mEntity = nullptr;
};