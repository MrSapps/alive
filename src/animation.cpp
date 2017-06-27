#include "animation.hpp"
#include "oddlib/anim.hpp"
#include "debug.hpp"
#include "physics.hpp"

Animation::AnimationSetHolder::AnimationSetHolder(std::shared_ptr<Oddlib::LvlArchive> sLvlPtr, std::shared_ptr<Oddlib::AnimationSet> sAnimSetPtr, u32 animIdx) : mLvlPtr(sLvlPtr), mAnimSetPtr(sAnimSetPtr)
{
    mAnim = mAnimSetPtr->AnimationAt(animIdx);
}

const Oddlib::Animation& Animation::AnimationSetHolder::Animation() const
{
    return *mAnim;
}

u32 Animation::AnimationSetHolder::MaxW() const
{
    return mAnimSetPtr->MaxW();
}

u32 Animation::AnimationSetHolder::MaxH() const
{
    return mAnimSetPtr->MaxH();
}

Animation::Animation(AnimationSetHolder anim, bool isPsx, bool scaleFrameOffsets, u32 defaultBlendingMode, const std::string& sourceDataSet) : mAnim(anim), mIsPsx(isPsx), mScaleFrameOffsets(scaleFrameOffsets), mSourceDataSet(sourceDataSet)
{
    switch (defaultBlendingMode)
    {
    case 0:
        mBlendingMode = AbstractRenderer::eBlendModes::eNormal;
        break;
    case 1:
        mBlendingMode = AbstractRenderer::eBlendModes::eB100F100;
        break;
    default:
        // TODO: Other required blending modes
        mBlendingMode = AbstractRenderer::eBlendModes::eNormal;
        LOG_WARNING("Unknown blending mode: " << defaultBlendingMode);
    }
}

s32 Animation::FrameCounter() const
{
    return mCounter;
}

bool Animation::Update()
{
    bool ret = false;
    mCounter++;
    if (mCounter >= mFrameDelay)
    {
        ret = true;
        mFrameDelay = mAnim.Animation().Fps(); // Because mFrameDelay is 1 initially and Fps() can be > 1
        mCounter = 0;
        mFrameNum++;
        if (mFrameNum >= mAnim.Animation().NumFrames())
        {
            if (mAnim.Animation().Loop())
            {
                mFrameNum = mAnim.Animation().LoopStartFrame();
            }
            else
            {
                mFrameNum = mAnim.Animation().NumFrames() - 1;
            }

            // Reached the final frame, animation has completed 1 cycle
            mCompleted = true;
        }

        // Are we *on* the last frame?
        mIsLastFrame = (mFrameNum == mAnim.Animation().NumFrames() - 1);
    }
    return ret;
}

bool Animation::IsLastFrame() const
{
    return mIsLastFrame;
}

bool Animation::IsComplete() const
{
    return mCompleted;
}

void Animation::Render(AbstractRenderer& rend, bool flipX, int layer, AbstractRenderer::eCoordinateSystem coordinateSystem /*= AbstractRenderer::eWorld*/) const
{
    // TODO: Position calculation should be refactored

    /*
    static std::string msg;
    std::stringstream s;
    s << "Render frame number: " << mFrameNum;
    if (s.str() != msg)
    {
    LOG_INFO(s.str());
    }
    msg = s.str();
    */

    const Oddlib::Animation::Frame& frame = mAnim.Animation().GetFrame(mFrameNum == -1 ? 0 : mFrameNum);

    f32 xFrameOffset = (mScaleFrameOffsets ? static_cast<f32>(frame.mOffX / kPcToPsxScaleFactor) : static_cast<f32>(frame.mOffX)) * mScale;
    const f32 yFrameOffset = static_cast<f32>(frame.mOffY) * mScale;

    const f32 xpos = static_cast<f32>(mXPos);
    const f32 ypos = static_cast<f32>(mYPos);

    if (flipX)
    {
        xFrameOffset = -xFrameOffset;
    }
    // Render sprite as textured quad
    const TextureHandle textureId = rend.CreateTexture(AbstractRenderer::eTextureFormats::eRGBA, frame.mFrame->w, frame.mFrame->h, AbstractRenderer::eTextureFormats::eRGBA, frame.mFrame->pixels, true);
    rend.TexturedQuad(
        textureId,
        xpos + xFrameOffset,
        ypos + yFrameOffset,
        static_cast<f32>(frame.mFrame->w) * (flipX ? -ScaleX() : ScaleX()),
        static_cast<f32>(frame.mFrame->h) * mScale,
        layer,
        ColourU8{ 255, 255, 255, 255 },
        AbstractRenderer::eNormal,
        coordinateSystem
    );
    rend.DestroyTexture(textureId);

    if (Debugging().mAnimBoundingBoxes)
    {
        // Render bounding box
        const ColourU8 boundingBoxColour{ 255, 0, 255, 255 };
        const f32 width = static_cast<f32>(std::abs(frame.mTopLeft.x - frame.mBottomRight.x)) * mScale;

        const glm::vec4 rectScreen(rend.WorldToScreenRect(xpos + (static_cast<f32>(flipX ? -frame.mTopLeft.x : frame.mTopLeft.x) * mScale),
            ypos + (static_cast<f32>(frame.mTopLeft.y) * mScale),
            flipX ? -width : width,
            static_cast<f32>(std::abs(frame.mTopLeft.y - frame.mBottomRight.y)) * mScale));

        rend.Rect(rectScreen.x, rectScreen.y, rectScreen.z, rectScreen.w,
            layer,
            boundingBoxColour,
            AbstractRenderer::eBlendModes::eNormal,
            AbstractRenderer::eCoordinateSystem::eScreen);
    }

    if (Debugging().mAnimDebugStrings)
    {
        // Render frame pos and frame number
        const glm::vec2 xyposScreen(rend.WorldToScreen(glm::vec2(xpos, ypos)));
        rend.Text(xyposScreen.x, xyposScreen.y,
            24.0f,
            (mSourceDataSet
                + " x: " + std::to_string(xpos)
                + " y: " + std::to_string(ypos)
                + " f: " + std::to_string(FrameNumber())
                ).c_str(),
            ColourU8{ 255,255,255,255 },
            AbstractRenderer::eLayers::eFmv,
            AbstractRenderer::eBlendModes::eNormal,
            AbstractRenderer::eCoordinateSystem::eScreen);
    }
}

void Animation::SetFrame(u32 frame)
{
    mCounter = 0;
    mFrameDelay = 1; // Force change frame on first Update()
    mFrameNum = frame;
    mIsLastFrame = false;
    mCompleted = false;
}

void Animation::Restart()
{
    mCounter = 0;
    mFrameDelay = 1; // Force change frame on first Update()
    mFrameNum = -1;
    mIsLastFrame = false;
    mCompleted = false;
}

bool Animation::Collision(s32 x, s32 y) const
{
    const Oddlib::Animation::Frame& frame = mAnim.Animation().GetFrame(FrameNumber());

    // TODO: Refactor rect calcs
    f32 xpos = mScaleFrameOffsets ? static_cast<f32>(frame.mOffX / kPcToPsxScaleFactor) : static_cast<f32>(frame.mOffX);
    f32 ypos = static_cast<f32>(frame.mOffY);

    ypos = mYPos + (ypos * mScale);
    xpos = mXPos + (xpos * mScale);

    f32 w = static_cast<f32>(frame.mFrame->w) * ScaleX();
    f32 h = static_cast<f32>(frame.mFrame->h) * mScale;


    return Physics::PointInRect(x, y, static_cast<s32>(xpos), static_cast<s32>(ypos), static_cast<s32>(w), static_cast<s32>(h));
}

void Animation::SetXPos(s32 xpos)
{
    mXPos = xpos;
}

void Animation::SetYPos(s32 ypos)
{
    mYPos = ypos;
}

s32 Animation::XPos() const
{
    return mXPos;
}

s32 Animation::YPos() const
{
    return mYPos;
}

u32 Animation::MaxW() const
{
    return static_cast<u32>(mAnim.MaxW()*ScaleX());
}

u32 Animation::MaxH() const
{
    return static_cast<u32>(mAnim.MaxH()*mScale);
}

s32 Animation::FrameNumber() const
{
    return mFrameNum;
}

u32 Animation::NumberOfFrames() const
{
    return mAnim.Animation().NumFrames();
}

void Animation::SetScale(f32 scale)
{
    mScale = scale;
}

f32 Animation::ScaleX() const
{
    // PC sprites have a bigger width as they are higher resolution
    return mIsPsx ? (mScale) : (mScale / kPcToPsxScaleFactor);
}

const /*static*/ f32 Animation::kPcToPsxScaleFactor = 1.73913043478f;
