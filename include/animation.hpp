#pragma once

#include "types.hpp"
#include "abstractrenderer.hpp"

namespace Oddlib
{
    class Animation;
    class LvlArchive;
    class AnimationSet;
}

class Animation
{
public:
    // Keeps the LVL and AnimSet shared pointers in scope for as long as the Animation lives.
    // On destruction if its the last instance of the lvl/animset the lvl will be closed and removed
    // from the cache, and the animset will be deleted/freed.
    struct AnimationSetHolder
    {
    public:
        AnimationSetHolder(std::shared_ptr<Oddlib::LvlArchive> sLvlPtr, std::shared_ptr<Oddlib::AnimationSet> sAnimSetPtr, u32 animIdx);
        const Oddlib::Animation& Animation() const;
        u32 MaxW() const;
        u32 MaxH() const;
    private:
        std::shared_ptr<Oddlib::LvlArchive> mLvlPtr;
        std::shared_ptr<Oddlib::AnimationSet> mAnimSetPtr;
        const Oddlib::Animation* mAnim;
    };
    Animation(const Animation&) = delete;
    Animation& operator = (const Animation&) = delete;
    Animation() = delete;
    Animation(AnimationSetHolder anim, bool isPsx, bool scaleFrameOffsets, u32 defaultBlendingMode, const std::string& sourceDataSet);
    s32 FrameCounter() const;
    bool Update();
    bool IsLastFrame() const;
    bool IsComplete() const;
    void Render(AbstractRenderer& rend, bool flipX, int layer, AbstractRenderer::eCoordinateSystem coordinateSystem = AbstractRenderer::eWorld, bool bDrawBoundingBox = false) const;
    void SetFrame(u32 frame);
    void Restart();
    bool Collision(s32 x, s32 y) const;
    void SetXPos(s32 xpos);
    void SetYPos(s32 ypos);
    s32 XPos() const;
    s32 YPos() const;
    u32 MaxW() const;
    u32 MaxH() const;
    s32 FrameNumber() const;
    u32 NumberOfFrames() const;
    void SetScale(f32 scale);
private:

    // 640 (pc xres) / 368 (psx xres) = 1.73913043478 scale factor
    const static f32 kPcToPsxScaleFactor;

    f32 ScaleX() const;

    AnimationSetHolder mAnim;
    bool mIsPsx = false;
    bool mScaleFrameOffsets = false;
    std::string mSourceDataSet;
    AbstractRenderer::eBlendModes mBlendingMode = AbstractRenderer::eBlendModes::eNormal;

    // The "FPS" of the animation, set to 1 first so that the first Update() brings us from frame -1 to 0
    u32 mFrameDelay = 1;

    // When >= mFrameDelay, mFrameNum is incremented
    u32 mCounter = 0;
    s32 mFrameNum = -1;

    s32 mXPos = 100;
    s32 mYPos = 100;
    f32 mScale = 1.0f;

    bool mIsLastFrame = false;
    bool mCompleted = false;
};
