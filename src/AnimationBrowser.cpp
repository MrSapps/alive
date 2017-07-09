#include "animationbrowser.hpp"
#include "abstractrenderer.hpp"
#include "oddlib/anim.hpp"
#include "oddlib/lvlarchive.hpp"
#include "fmv.hpp"
#include "sound.hpp"
#include "gridmap.hpp"

void AnimationBrowser::Update(const InputState& input, CoordinateSpace& coords)
{
    if (Debugging().mBrowserUi.animationBrowserOpen)
    {
        RenderAnimationSelector(coords);
    }

    for (auto& anim : mLoadedAnims)
    {
        anim->Update();
    }

    const glm::vec2 mouseWorldPos = coords.ScreenToWorld(glm::vec2(input.mMousePosition.mX, input.mMousePosition.mY));

    // Set the "selected" animation
    if (!mSelected && input.mMouseButtons[0].IsPressed())
    {
        for (auto& anim : mLoadedAnims)
        {
            if (anim->Collision(static_cast<s32>(mouseWorldPos.x), static_cast<s32>(mouseWorldPos.y)))
            {
                mSelected = anim.get();
                mXDelta = (static_cast<s32>(anim->XPos()-mouseWorldPos.x));
                mYDelta = (static_cast<s32>(anim->YPos()-mouseWorldPos.y));
                return;
            }
        }
    }

    // Move the "selected" animation
    if (mSelected && input.mMouseButtons[0].IsDown())
    {
        LOG_INFO("Move selected to " << mouseWorldPos.x+ mXDelta << "," << mouseWorldPos.y+ mYDelta);
        mSelected->SetXPos(static_cast<s32>(mouseWorldPos.x+ mXDelta));
        mSelected->SetYPos(static_cast<s32>(mouseWorldPos.y+ mYDelta));
    }

    if (input.mMouseButtons[0].IsReleased())
    {
        mSelected = nullptr;
    }

    // When the right button is released delete the "selected" animation
    if (input.mMouseButtons[1].IsPressed())
    {
        for (auto it = mLoadedAnims.begin(); it != mLoadedAnims.end(); it++)
        {
            if ((*it)->Collision(static_cast<s32>(mouseWorldPos.x), static_cast<s32>(mouseWorldPos.y)))
            {
                mLoadedAnims.erase(it);
                break;
            }
        }
        mSelected = nullptr;
    }
}

AnimationBrowser::AnimationBrowser(ResourceLocator& resMapper) 
    : mResourceLocator(resMapper)
{

}

void AnimationBrowser::Render(AbstractRenderer& renderer)
{
    for (auto& anim : mLoadedAnims)
    {
        if (mDebugResetAnimStates)
        {
            anim->Restart();
        }
        anim->Render(renderer, false, AbstractRenderer::eForegroundLayer0, AbstractRenderer::eWorld);
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
                    auto anim = mResourceLocator.LocateAnimation(resourceName, dataSetName);
                    if (anim)
                    {
                        anim->SetXPos(static_cast<s32>(coords.CameraPosition().x));
                        anim->SetYPos(static_cast<s32>(coords.CameraPosition().y));
                        mLoadedAnims.push_back(std::move(anim));
                    }
                }
            }
        }
    }
    ImGui::End();
}
