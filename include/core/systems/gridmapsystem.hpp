#pragma once

#include <memory>

#include "core/system.hpp"
#include "core/component.hpp"


class ResourceLocator;
class CoordinateSpace;
class AbstractRenderer;
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
    GridmapSystem(CoordinateSpace& coords);

public:
    void OnLoad() final;

public:
    void MoveToCamera(const char* cameraName);
    void MoveToCamera(ResourceLocator& locator, u32 xIndex, u32 yIndex);
    bool LoadMap(const PathInformation& pathInfo);
    void UnloadMap(AbstractRenderer& renderer) const;

private:
    std::unique_ptr<class GridMap> mGridMap;
    CoordinateSpace& mCoords;
};

class GridMapScreenComponent : public Component
{
public:
    DECLARE_COMPONENT(GridMapScreenComponent);
    GridMapScreenComponent();
public:
    void Render(AbstractRenderer& rend, float x, float y, float w, float h) const;
    void LoadCamera(ResourceLocator& locator, const std::string& name);
private:
    std::unique_ptr<Oddlib::IBits> mBits;
};
