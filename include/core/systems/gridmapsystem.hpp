#pragma once

#include <memory>

#include "core/system.hpp"
#include "core/component.hpp"

class ResourceLocator;
class CoordinateSpace;
class AbstractRenderer;
class CameraSystem;
struct PathInformation;

namespace Oddlib
{
    class IBits;
}

class GridmapSystem final : public System
{
public:
    DECLARE_SYSTEM(GridmapSystem);

public:
    explicit GridmapSystem(CoordinateSpace& coords);

public:
    void OnResolveDependencies() final;

public:
    bool LoadMap(const PathInformation& pathInfo);
    void UnLoadMap();

public:
    void MoveToCamera(ResourceLocator& locator, const char* cameraName);
    void MoveToCamera(ResourceLocator& locator, u32 xIndex, u32 yIndex);

public:
    void AddGridScreen(CameraSystem* cameraSystem, ResourceLocator& locator, u32 xIndex, u32 yIndex);
    void LoadAllGridScreens(ResourceLocator& locator);
    void UnloadAllGridScreens();

public:
    u32 GetCurrentGridScreenX() const;
    u32 GetCurrentGridScreenY() const;

public:
    u32 mCurrentGridScreenX = 0;
    u32 mCurrentGridScreenY = 0;

private:
    CoordinateSpace& mCoords;
    std::unique_ptr<class GridMap> mGridMap;
};

class GridMapScreenComponent : public Component
{
public:
    DECLARE_COMPONENT(GridMapScreenComponent);

public:
    GridMapScreenComponent();

public:
    void Render(AbstractRenderer& rend, float x, float y, float w, float h) const;
    void LoadCamera(ResourceLocator& locator, const std::string& name);

private:
    std::unique_ptr<Oddlib::IBits> mBits;
};
