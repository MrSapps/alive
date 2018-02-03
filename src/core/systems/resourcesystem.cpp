#include "core/systems/resourcesystem.hpp"

DEFINE_SYSTEM(ResourceSystem);

ResourceSystem::ResourceSystem(ResourceLocator& resLoc) : mResourceLocator(&resLoc)
{

}

void ResourceSystem::Update()
{

}

ResourceLocator* ResourceSystem::GetResourceLocator() const
{
    return mResourceLocator;
}