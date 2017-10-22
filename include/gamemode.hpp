#pragma once

class WorldState;
class InputState;
class CoordinateSpace;
class AbstractRenderer;

class GameMode
{
public:
    GameMode(WorldState& mapState);

    void Update(const InputState& input, CoordinateSpace& coords);
    void Render(AbstractRenderer& rend) const;
private:
    WorldState& mWorldState;
};
