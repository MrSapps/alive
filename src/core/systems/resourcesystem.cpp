#include "core/systems/resourcesystem.hpp"

DEFINE_SYSTEM(ResourceSystem);

ResourceSystem::ResourceSystem(ResourceLocator& resLoc) : mResourceLocator(&resLoc)
{

}

ResourceLocator* ResourceSystem::GetResourceLocator() const
{
    return mResourceLocator;
}