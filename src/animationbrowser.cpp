#include "animationbrowser.hpp"
#include "abstractrenderer.hpp"
#include "oddlib/anim.hpp"
#include "oddlib/lvlarchive.hpp"
#include "fmv.hpp"
#include "sound.hpp"
#include "gridmap.hpp"
#include "debug.hpp"
#include "resourcemapper.hpp"
#include "core/components/transformcomponent.hpp"
#include "core/components/animationcomponent.hpp"

AnimationBrowser::AnimationBrowser(EntityManager& entityManager, ResourceLocator& resMapper) : mEntityManager(entityManager), mResourceLocator(resMapper)
{

}

void AnimationBrowser::Update(const InputReader& input, CoordinateSpace& coords)
{
    if (Debugging().mBrowserUi.animationBrowserOpen)
    {
        RenderAnimationSelector(coords);
    }

    const glm::vec2 mouseWorldPos = coords.ScreenToWorld(glm::vec2(input.MousePos().mX, input.MousePos().mY));

    // Set the "selected" animation
    if (!mSelected && input.MouseButton(MouseButtons::eLeft).Pressed())
    {
        for (auto& entity : mLoadedAnims)
        {
            auto animation = entity.GetComponent<AnimationComponent>()->mAnimation;
            if (animation && animation->Collision(static_cast<s32>(mouseWorldPos.x), static_cast<s32>(mouseWorldPos.y)))
            {
                mSelected = entity;
                mXDelta = (static_cast<s32>(animation->XPos() - mouseWorldPos.x));
                mYDelta = (static_cast<s32>(animation->YPos() - mouseWorldPos.y));
                return;
            }
        }
    }

    // Move the "selected" animation
    if (mSelected && input.MouseButton(MouseButtons::eLeft).Held())
    {
        LOG_INFO("Move selected to " << mouseWorldPos.x + mXDelta << "," << mouseWorldPos.y + mYDelta);
        mSelected.With<TransformComponent, AnimationComponent>([&](auto transformComponent, auto animationComponent)
        {
            transformComponent->Set(mouseWorldPos.x + mXDelta, mouseWorldPos.y + mYDelta);
            animationComponent->mAnimation->SetXPos(static_cast<s32>(mouseWorldPos.x + mXDelta)); // TODO: this is still used in collision checking for animations
            animationComponent->mAnimation->SetYPos(static_cast<s32>(mouseWorldPos.y + mYDelta)); // TODO: this is still used in collision checking for animations
        });
    };

    if (input.MouseButton(MouseButtons::eLeft).Released())
    {
        mSelected = nullptr;
    }

    // When the right button is released delete the "selected" animation
    if (input.MouseButton(MouseButtons::eRight).Pressed())
    {
        for (auto it = mLoadedAnims.begin(); it != mLoadedAnims.end(); it++)
        {
            auto animation = it->GetComponent<AnimationComponent>()->mAnimation;
            if (animation->Collision(static_cast<s32>(mouseWorldPos.x), static_cast<s32>(mouseWorldPos.y)))
            {
				it->Destroy();
                mLoadedAnims.erase(it);
                break;
            }
        }
        mSelected = nullptr;
    }
}

void AnimationBrowser::Render(AbstractRenderer& renderer)
{
    for (auto& entity : mLoadedAnims)
    {
        auto animation = entity.GetComponent<AnimationComponent>()->mAnimation;
        if (animation)
        {
            if (mDebugResetAnimStates)
            {
                animation->Restart();
            }
            animation->Render(renderer, false, AbstractRenderer::eEditor, AbstractRenderer::eWorld);
        }
    }
}

void AnimationBrowser::RenderAnimationSelector(CoordinateSpace& coords)
{
    mDebugResetAnimStates = false;
    if (ImGui::Begin("Animations"))
    {
        if (ImGui::Button("Reset states"))
        {
            mDebugResetAnimStates = true;
        }

        if (ImGui::Button("Clear all"))
        {
            mLoadedAnims.clear();
        }

        static char datasetFilterString[64] = {};
        ImGui::InputText("Data set filter", datasetFilterString, sizeof(datasetFilterString));

        static char nameFilterString[64] = {};
        ImGui::InputText("Name filter", nameFilterString, sizeof(nameFilterString));

        auto animsToLoad = mResourceLocator.DebugUi(datasetFilterString, nameFilterString);
        for (const auto& res : animsToLoad)
        {
            if (std::get<0>(res))
            {
                const char* dataSetName = std::get<0>(res);
                const char* resourceName = std::get<1>(res);
                bool load = std::get<2>(res);
                if (load)
                {
                    auto entity = mEntityManager.CreateEntityWith<TransformComponent, AnimationComponent>();
                    entity.With<TransformComponent, AnimationComponent>([&](auto transformComponent, auto animationComponent)
                    {
                        transformComponent->Set(coords.CameraPosition().x, coords.CameraPosition().y);
                        animationComponent->Change(resourceName, dataSetName);
                        if (animationComponent->mAnimation)
                        {
                            animationComponent->mAnimation->SetXPos(static_cast<s32>(coords.CameraPosition().x)); // TODO: this is still used in collision checking for animations
                            animationComponent->mAnimation->SetYPos(static_cast<s32>(coords.CameraPosition().y)); // TODO: this is still used in collision checking for animations
                            mLoadedAnims.emplace_back(entity);
                        }
                        else
                        {
                            entity.Destroy();
                        }
                    });
                }
            }
        }
    }
    ImGui::End();
}