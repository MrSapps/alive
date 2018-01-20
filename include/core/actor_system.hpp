#pragma once

class Actor;

class ActorSystem
{
public:
    enum class ActorLevels
    {
        eCamera,
        eMapBackground,
        eMapForeground,
        eMenu,
    };
    void AddActor(ActorLevels level, Actor* pActor);
    void UpdateActors();
    void RenderActors();
private:
};
