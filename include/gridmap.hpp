#pragma once

#include <memory>
#include <vector>
#include <deque>

class GridScreen
{
public:

private:
    std::vector<std::unique_ptr<class MapObject>> mObjects;
};

class GridMap
{
public:
    GridMap(int xSize, int ySize, int gridX, int gridY);

    void Add();
    void Remove();

private:
    std::deque<std::deque<std::unique_ptr<GridScreen>>> mScreens;
    std::vector<std::unique_ptr<class MapObject>> mObjects;
};
