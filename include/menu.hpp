#pragma once

class AbstractRenderer;
class EntityManager;
class ResourceLocator;

class Menu
{
public:
    Menu(EntityManager& em, ResourceLocator& locator);

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
    enum class States
    {
        eSetFirstScreen,
        eRunning
    };
    States mState = States::eSetFirstScreen;
    EntityManager& mEm;
    ResourceLocator& mLocator;
};
