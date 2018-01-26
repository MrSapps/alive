#pragma once

#include "resourcemapper.hpp"
#include "core/component.hpp"

class ResourceLocatorSystem : public Component
{
public:
    DECLARE_COMPONENT(ResourceLocatorSystem);

public:
    void Initialize(ResourceLocator& state);

public:
    ResourceLocator* GetResourceLocator() const;

private:
    ResourceLocator* mResourceLocator = nullptr;
};