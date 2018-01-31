#include "core/systems/resourcelocatorsystem.hpp"

DEFINE_SYSTEM(ResourceLocatorSystem)

ResourceLocatorSystem::ResourceLocatorSystem(ResourceLocator& resLoc) : mResourceLocator(&resLoc)
{

}

ResourceLocator* ResourceLocatorSystem::GetResourceLocator() const
{
    return mResourceLocator;
}

void ResourceLocatorSystem::Update()
{

}