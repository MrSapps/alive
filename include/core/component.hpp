#pragma once

#include <cstdint>

#define DEFINE_COMPONENT(NAME) constexpr const char* NAME::ComponentName;
#define DECLARE_COMPONENT(NAME) static constexpr const char* ComponentName{#NAME};

class Entity;

class Component
{
public:
    DECLARE_COMPONENT(Component)

public:
    friend Entity;

public:
    virtual ~Component() = 0;

protected:
    Entity* mEntity = nullptr;
};