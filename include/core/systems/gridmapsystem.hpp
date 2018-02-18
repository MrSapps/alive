#pragma once

#include <memory>

#include "core/system.hpp"
#include "core/component.hpp"

class GridScreen;
class ResourceLocator;
class CoordinateSpace;
class AbstractRenderer;
struct PathInformation;

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
    void MoveToCamera(u32 xIndex, u32 yIndex);
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

public:
    void Render(AbstractRenderer& rend, float x, float y, float w, float h) const;

private:
    std::unique_ptr<GridScreen> mScreen;
};
