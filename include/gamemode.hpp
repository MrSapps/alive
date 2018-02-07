#pragma once

#include "types.hpp"

class WorldState;
class InputReader;
class CoordinateSpace;
class AbstractRenderer;

class GameMode
{
public:
    GameMode(WorldState& mapState);

public:
    enum GameModeStates
    {
        eRunning,
        eMenu,
        ePaused
    };

public:
    void Update(const InputReader& input, CoordinateSpace& coords);
    void Render(AbstractRenderer& rend) const;

public:
    GameModeStates State() const;

private:
    enum class MenuStates
    {
        eInit,
        eCameraRoll,
        eFmv,
        eUserMenu,
    };

private:
    void UpdateMenu(const InputReader& input, CoordinateSpace& coords);

private:
    MenuStates mMenuState = MenuStates::eInit;
    WorldState& mWorldState;
    GameModeStates mState = eRunning;
};
