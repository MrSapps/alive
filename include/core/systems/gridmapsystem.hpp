#pragma once

#include "core/system.hpp"
#include "core/component.hpp"
#include <memory>

struct PathInformation;
class CoordinateSpace;
class ResourceLocator;
class AbstractRenderer;
class GridScreen;

class GridmapSystem final : public System
{
public:
    DECLARE_SYSTEM(GridmapSystem);
    GridmapSystem(CoordinateSpace& coords);
public:
    void MoveToCamera(const char* cameraName);
    void OnLoad() final;
    void Update() final;
    bool LoadMap(const PathInformation& pathInfo, ResourceLocator& mLocator);
    void UnloadMap(AbstractRenderer& renderer) const;
private:
    std::unique_ptr<class GridMap> mGridMap;
    CoordinateSpace& mCoords;
};


class GridMapScreenComponent : public Component
{
public:
    DECLARE_COMPONENT(GridMapScreenComponent);
    void Render(AbstractRenderer& rend) const;
private:
    std::unique_ptr<GridScreen> mScreen;
};
