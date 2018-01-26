#include "core/systems/resourcelocatorsystem.hpp"

DEFINE_COMPONENT(ResourceLocatorSystem);

void ResourceLocatorSystem::Initialize(ResourceLocator& resLoc)
{
    mResourceLocator = &resLoc;
}

ResourceLocator* ResourceLocatorSystem::GetResourceLocator() const
{
    return mResourceLocator;
}