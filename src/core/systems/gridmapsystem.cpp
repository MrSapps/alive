#include "gridmap.hpp"
#include "core/systems/gridmapsystem.hpp"
#include "core/systems/camerasystem.hpp"
#include "core/entity.hpp"
#include "core/components/transformcomponent.hpp"
#include "resourcemapper.hpp"
#include "oddlib/bits_factory.hpp"

DEFINE_SYSTEM(GridmapSystem);

GridmapSystem::GridmapSystem(CoordinateSpace& coords) : mCoords(coords)
{

}

void GridmapSystem::OnResolveDependencies()
{
    mGridMap = std::make_unique<GridMap>(mCoords, *mManager);
}

void GridmapSystem::MoveToCamera(ResourceLocator& locator, const char* cameraName)
{

    for (u32 x = 0; x < mGridMap->XSize(); x++)
    {
        for (u32 y = 0; y < mGridMap->YSize(); y++)
        {
            if (mGridMap->GetGridScreen(x, y)->mCameraAndObjects.mName == cameraName)
            {
                MoveToCamera(locator, x, y);
                return;
            }
        }
    }
}

void GridmapSystem::MoveToCamera(ResourceLocator& locator, u32 xIndex, u32 yIndex)
{
    // Remove all existing grid map screens.. there can only be 1 rendered/active at a time
    // during "game" mode.
    UnloadAllGridScreens();

    auto cameraSystem = mManager->GetSystem<CameraSystem>();
    AddGridScreen(cameraSystem, locator, xIndex, yIndex);
}

bool GridmapSystem::LoadMap(const PathInformation& pathInfo)
{
    return mGridMap->LoadMap(pathInfo);
}

void GridmapSystem::UnLoadMap()
{
    UnloadAllGridScreens();
}

void GridmapSystem::AddGridScreen(CameraSystem* cameraSystem, ResourceLocator& locator, u32 xIndex, u32 yIndex)
{
    GridScreenData* pData = mGridMap->GetGridScreen(xIndex, yIndex);
    assert(pData);

    auto entity = mManager->CreateEntityWith<GridMapScreenComponent, TransformComponent>();

    auto gridMapScreen = entity.GetComponent<GridMapScreenComponent>();
    gridMapScreen->LoadCamera(locator, pData->mCameraAndObjects.mName);

    auto pTransform = entity.GetComponent<TransformComponent>();
    pTransform->Set(xIndex * cameraSystem->mCameraBlockSize.x, yIndex * cameraSystem->mCameraBlockSize.y);
}

void GridmapSystem::LoadAllGridScreens(ResourceLocator& locator)
{
    auto cameraSystem = mManager->GetSystem<CameraSystem>();
    for (u32 x = 0; x < mGridMap->XSize(); x++)
    {
        for (u32 y = 0; y < mGridMap->YSize(); y++)
        {
            AddGridScreen(cameraSystem, locator, x, y);
        }
    }
}

void GridmapSystem::UnloadAllGridScreens()
{
    for (auto entity : mManager->With<GridMapScreenComponent>())
    {
        entity.Destroy();
    }
}

u32 GridmapSystem::GetCurrentGridScreenX() const
{
    return mCurrentGridScreenX;
}

u32 GridmapSystem::GetCurrentGridScreenY() const
{
    return mCurrentGridScreenY;
}

DEFINE_COMPONENT(GridMapScreenComponent);

GridMapScreenComponent::GridMapScreenComponent()
{

}

void GridMapScreenComponent::Render(AbstractRenderer& rend, float x, float y, float w, float h) const
{
    if (mBits)
    {
        SDL_Surface* pBackgroundImage = mBits->GetSurface();
        if (pBackgroundImage)
        {
            TextureHandle backgroundText = rend.CreateTexture(AbstractRenderer::eTextureFormats::eRGB,
                static_cast<u32>(pBackgroundImage->w),
                static_cast<u32>(pBackgroundImage->h),
                AbstractRenderer::eTextureFormats::eRGB,
                pBackgroundImage->pixels, true);
            rend.TexturedQuad(backgroundText, x, y, w, h, AbstractRenderer::eForegroundLayer0, ColourU8{ 255, 255, 255, 255 });
            rend.DestroyTexture(backgroundText);

            if (mBits->GetFg1())
            {
                SDL_Surface* fg1Surf = mBits->GetFg1()->GetSurface();
                if (fg1Surf)
                {
                    TextureHandle fg1Texture = rend.CreateTexture(AbstractRenderer::eTextureFormats::eRGBA,
                        static_cast<u32>(fg1Surf->w),
                        static_cast<u32>(fg1Surf->h),
                        AbstractRenderer::eTextureFormats::eRGBA,
                        fg1Surf->pixels, true);
                    rend.TexturedQuad(fg1Texture, x, y, w, h, AbstractRenderer::eForegroundLayer1, ColourU8{ 255, 255, 255, 255 });
                    rend.DestroyTexture(fg1Texture);
                }
            }
        }
    }
}

void GridMapScreenComponent::LoadCamera(ResourceLocator& locator, const std::string& name)
{
    mBits = locator.LocateCamera(name).get();
}
