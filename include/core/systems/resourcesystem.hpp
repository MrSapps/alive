#pragma once

#include "resourcemapper.hpp"
#include "core/system.hpp"

class ResourceSystem final : public System
{
public:
    DECLARE_SYSTEM(ResourceSystem);

public:
    explicit ResourceSystem(ResourceLocator& state);

public:
    ResourceLocator* GetResourceLocator() const;

private:
    ResourceLocator* mResourceLocator = nullptr;
};