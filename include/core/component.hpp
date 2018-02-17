#pragma once

#include <memory>
#include <string>
#include <iosfwd>

#define DECLARE_COMPONENT(NAME) static constexpr const char* ComponentName{#NAME}; virtual std::string GetComponentName() const override
#define DEFINE_COMPONENT(NAME) std::string NAME::GetComponentName() const { return NAME::ComponentName; } constexpr const char* NAME::ComponentName

#define DECLARE_ROOT_COMPONENT(NAME) static constexpr const char* ComponentName{#NAME}; virtual std::string GetComponentName() const
#define DEFINE_ROOT_COMPONENT(NAME) std::string NAME::GetComponentName() const { return NAME::ComponentName; } constexpr const char* NAME::ComponentName

class Entity;
class EntityManager;

class Component
{
public:
    DECLARE_ROOT_COMPONENT(Component);

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
   Entity mEntity;
};

#undef DECLARE_ROOT_COMPONENT