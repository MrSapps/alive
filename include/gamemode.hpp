#pragma once

#include "gridmap.hpp"

class GameMode
{
public:
    NO_MOVE_OR_MOVE_ASSIGN(GameMode);

    GameMode(GridMapState& mapState);

    void Update(const InputState& input, CoordinateSpace& coords);
    void Render(AbstractRenderer& rend) const;
private:
    GridMapState& mMapState;
};
