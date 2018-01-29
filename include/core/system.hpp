#pragma once

#include <string>

#define DECLARE_SYSTEM(NAME) static constexpr const char* SystemName{#NAME}; virtual std::string GetSystemName() const override
#define DEFINE_SYSTEM(NAME) constexpr const char* NAME::SystemName; std::string NAME::GetSystemName() const { return NAME::SystemName; }

#define DECLARE_ROOT_SYSTEM(NAME) static constexpr const char* SystemName{#NAME}; virtual std::string GetSystemName() const
#define DEFINE_ROOT_SYSTEM(NAME) constexpr const char* NAME::SystemName; std::string NAME::GetSystemName() const { return NAME::SystemName; }

class EntityManager;

class System
{
public:
    DECLARE_ROOT_SYSTEM(System);

public:
    friend EntityManager;

public:
    virtual ~System() = 0;

public:
    virtual void Update() = 0;

protected:
    virtual void OnLoad();
    virtual void OnResolveDependencies();

protected:
    EntityManager* mManager = nullptr;
};

#undef DECLARE_ROOT_SYSTEM