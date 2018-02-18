#include "menu.hpp"
#include "core/entitymanager.hpp"
#include "core/systems/camerasystem.hpp"
#include "core/systems/gridmapsystem.hpp"

Menu::Menu(EntityManager& em, ResourceLocator& locator)
    : mEm(em), mLocator(locator)
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
    if (mState == States::eSetFirstScreen)
    {
        auto camPos = mEm.GetSystem<GridmapSystem>()->MoveToCamera(mLocator, "STP01C25.CAM");

        const auto cameraSystem = mEm.GetSystem<CameraSystem>();
        cameraSystem->SetGameCameraToCameraAt(camPos.first, camPos.second);

        mState = States::eRunning;
    }

}

void Menu::Render(AbstractRenderer& /*rend*/)
{

}
