#pragma once

class AbstractRenderer;
class EntityManager;

class Menu
{
public:
    Menu(EntityManager& em);

    void ToBootSequnce();
    void ToIntroSequnce();

    void ToLevelSelect();
    void ToDemoSelect();
    void ToMovieSelect();

    void ToAbeGameSpeak();
    void ToSligGameSpeak();
    void ToGlukkonGameSpeak();
    void ToScrabGameSpeak();
    void ToParamiteGameSpeak();

    void ToGameSpeak();
    void ToBegin();
    void ToLoad();
    void ToOptions();
    void ToQuit();

    void Update();
    void Render(AbstractRenderer& rend);

private:

    EntityManager& mEm;
};
