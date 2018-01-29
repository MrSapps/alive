#pragma once

#include "resourcemapper.hpp"
#include "core/system.hpp"

class ResourceLocatorSystem : public System
{
public:
    DECLARE_SYSTEM(ResourceLocatorSystem);

public:
    explicit ResourceLocatorSystem(ResourceLocator& state);

public:
    void Update() final;

public:
    ResourceLocator* GetResourceLocator() const;

private:
    ResourceLocator* mResourceLocator = nullptr;
};