#pragma once

#include "types.hpp"

class World;
class InputReader;
class CoordinateSpace;
class AbstractRenderer;

class GameMode
{
public:
    GameMode(World& mapState);

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
    World& mWorldState;
    MenuStates mMenuState = MenuStates::eInit;
    GameModeStates mState = eRunning;
};
