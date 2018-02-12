#include "core/components/transformcomponent.hpp"
#include "core/components/animationcomponent.hpp"
#include "core/systems/collisionsystem.hpp"
#include "core/systems/camerasystem.hpp"
#include "core/systems/debugsystem.hpp"
#include "core/entitymanager.hpp"

DEFINE_SYSTEM(DebugSystem);

void DebugSystem::Update()
{

}

void DebugSystem::Render(AbstractRenderer& rend) const
{
    if (Debugging().mGrid)
    {
        RenderGrid(rend);
    }
    if (Debugging().mRayCasts)
    {
        auto cameraTarget = mManager->GetSystem<CameraSystem>()->mTarget;
        if (cameraTarget)
        {
            cameraTarget.With<TransformComponent, AnimationComponent>([this, &rend](auto transform, auto animation)
            {
                RenderRaycast(rend,
                    glm::vec2(transform->GetX(), transform->GetY()),
                    glm::vec2(transform->GetX(), transform->GetY() + 500),
                    CollisionLine::eLineTypes::eFloor,
                    glm::vec2(0, -10)); // -10 so when we are *ON* a line you can see something
                RenderRaycast(rend,
                    glm::vec2(transform->GetX(), transform->GetY() - 2),
                    glm::vec2(transform->GetX(), transform->GetY() - 60),
                    CollisionLine::eLineTypes::eCeiling,
                    glm::vec2(0, 0));
                if (animation->mFlipX)
                {
                    RenderRaycast(rend,
                        glm::vec2(transform->GetX(), transform->GetY() - 20),
                        glm::vec2(transform->GetX() - 25, transform->GetY() - 20), CollisionLine::eLineTypes::eWallLeft);
                    RenderRaycast(rend,
                        glm::vec2(transform->GetX(), transform->GetY() - 50),
                        glm::vec2(transform->GetX() - 25, transform->GetY() - 50), CollisionLine::eLineTypes::eWallLeft);
                }
                else
                {
                    RenderRaycast(rend,
                        glm::vec2(transform->GetX(), transform->GetY() - 20),
                        glm::vec2(transform->GetX() + 25, transform->GetY() - 20), CollisionLine::eLineTypes::eWallRight);
                    RenderRaycast(rend,
                        glm::vec2(transform->GetX(), transform->GetY() - 50),
                        glm::vec2(transform->GetX() + 25, transform->GetY() - 50), CollisionLine::eLineTypes::eWallRight);
                }
            });
        }
    }
    if (Debugging().mObjectBoundingBoxes)
    {
        mManager->With<TransformComponent>([&rend](auto, auto transformComponent)
        {
            glm::vec2 topLeft = glm::vec2(transformComponent->GetX(), transformComponent->GetY());
            glm::vec2 bottomRight = glm::vec2(transformComponent->GetX() + transformComponent->GetWidth(), transformComponent->GetY() + transformComponent->GetHeight());
            glm::vec2 objPos = rend.WorldToScreen(glm::vec2(topLeft.x, topLeft.y));
            glm::vec2 objSize = rend.WorldToScreen(glm::vec2(bottomRight.x, bottomRight.y)) - objPos;

            rend.Rect(
                objPos.x, objPos.y,
                objSize.x, objSize.y,
                AbstractRenderer::eLayers::eEditor, ColourU8{ 255, 0, 255, 255 }, AbstractRenderer::eNormal, AbstractRenderer::eScreen);
        });
    }
}

void DebugSystem::RenderGrid(AbstractRenderer& rend) const
{
    const auto gridLineCountX = static_cast<int>((rend.ScreenSize().x / CameraSystem::kEditorGridSizeX));
    for (int x = -gridLineCountX; x < gridLineCountX; x++)
    {
        const glm::vec2 worldPos(rend.CameraPosition().x + (x * CameraSystem::kEditorGridSizeX) - (static_cast<int>(rend.CameraPosition().x) % CameraSystem::kEditorGridSizeX), 0);
        const glm::vec2 screenPos = rend.WorldToScreen(worldPos);
        rend.Line(ColourU8{ 255, 255, 255, 30 }, screenPos.x, 0, screenPos.x, static_cast<f32>(rend.Height()), 2.0f, AbstractRenderer::eLayers::eEditor, AbstractRenderer::eNormal, AbstractRenderer::eScreen);
    }

    const auto gridLineCountY = static_cast<int>((rend.ScreenSize().y / CameraSystem::kEditorGridSizeY));
    for (int y = -gridLineCountY; y < gridLineCountY; y++)
    {
        const glm::vec2 screenPos = rend.WorldToScreen(glm::vec2(0, rend.CameraPosition().y + (y * CameraSystem::kEditorGridSizeY) - (static_cast<int>(rend.CameraPosition().y) % CameraSystem::kEditorGridSizeY)));
        rend.Line(ColourU8{ 255, 255, 255, 30 }, 0, screenPos.y, static_cast<f32>(rend.Width()), screenPos.y, 2.0f, AbstractRenderer::eLayers::eEditor, AbstractRenderer::eNormal, AbstractRenderer::eScreen);
    }
}

void DebugSystem::RenderRaycast(AbstractRenderer& rend, const glm::vec2& from, const glm::vec2& to, CollisionLine::eLineTypes lineType, const glm::vec2& fromDrawOffset) const
{
    auto collisionSystem = mManager->GetSystem<CollisionSystem>();
    auto hit = collisionSystem->Raycast(from, to, { lineType });
    if (hit)
    {
        const glm::vec2 fromDrawPos = rend.WorldToScreen(from + fromDrawOffset);
        const glm::vec2 hitPos = rend.WorldToScreen(hit.Point());

        rend.Line(ColourU8{ 255, 0, 255, 255 },
            fromDrawPos.x, fromDrawPos.y,
            hitPos.x, hitPos.y,
            2.0f,
            AbstractRenderer::eLayers::eEditor,
            AbstractRenderer::eBlendModes::eNormal,
            AbstractRenderer::eCoordinateSystem::eScreen);
    }
}