#pragma once

#include "types.hpp"

class World;
class InputReader;
class CoordinateSpace;
class AbstractRenderer;

class GameMode
{
public:
    explicit GameMode(World& world);

public:
    enum class States
    {
        eRunning,
        eMenu,
        ePaused
    };
    enum class MenuStates
    {
        eInit,
        eCameraRoll,
        eFmv,
        eUserMenu,
    };

public:
    void FromEditorMode(u32 x, u32 y);

public:
    void Render(AbstractRenderer& rend) const;
    void Update(const InputReader& input, CoordinateSpace& coords);
    void UpdateMenu(const InputReader& input, CoordinateSpace& coords);

public:
    States State() const;
    MenuStates MenuState() const;

private:
    World& mWorld;
    States mState = States::eRunning;
    MenuStates mMenuState = MenuStates::eInit;
};
