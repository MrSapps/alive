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
    enum GameModeStates
    {
        eRunning,
        eMenu,
        ePaused
    };
    GameModeStates State() const { return mState; }
private:
    GameModeStates mState = eRunning;
    WorldState& mWorldState;
};
