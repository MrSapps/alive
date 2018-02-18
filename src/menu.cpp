#include "menu.hpp"
#include "core/entitymanager.hpp"
#include "core/systems/camerasystem.hpp"
#include "core/systems/gridmapsystem.hpp"

Menu::Menu(EntityManager& /*em*/) //: mEm(em)
{

}

void Menu::ToBootSequnce()
{

}

void Menu::ToIntroSequnce()
{

}

void Menu::ToLevelSelect()
{

}

void Menu::ToDemoSelect()
{

}

void Menu::ToMovieSelect()
{

}

void Menu::ToAbeGameSpeak()
{

}

void Menu::ToSligGameSpeak()
{

}

void Menu::ToGlukkonGameSpeak()
{

}

void Menu::ToScrabGameSpeak()
{

}

void Menu::ToParamiteGameSpeak()
{

}

void Menu::ToGameSpeak()
{

}

void Menu::ToBegin()
{

}

void Menu::ToLoad()
{

}

void Menu::ToOptions()
{

}

void Menu::ToQuit()
{

}

void Menu::Update()
{
    //mEm.GetSystem<GridmapSystem>()->MoveToCamera( "STP01C25.CAM");

    //const auto cameraSystem = mEm.GetSystem<CameraSystem>();
    //cameraSystem->SetGameCameraToCameraAt(cameraSystem->CurrentCameraX(), cameraSystem->CurrentCameraY());
}

void Menu::Render(AbstractRenderer& /*rend*/)
{

}
