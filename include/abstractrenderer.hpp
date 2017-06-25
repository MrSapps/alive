#pragma once

#include "types.hpp"
#include <vector>

#include <glm/glm.hpp>
#include <glm/vec3.hpp> // glm::vec3
#include <glm/vec4.hpp> // glm::vec4
#include <glm/mat4x4.hpp> // glm::mat4
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/compatibility.hpp>
#include "logger.hpp"
#include <memory>
#include "imgui/imgui.h"

struct ColourU8
{
    u8 r, g, b, a;
    u32 To32Bit() const
    {
        return (r) | (g << 8) | (b << 16) | (a << 24);
    }
};

class TextureHandle
{
public:
    void* mData = nullptr;
    bool IsValid() const { return mData != nullptr; }
};

// TODO: Pass array ref
inline void MatrixLerp(float* from, float* to, float speed)
{
    for (int m = 0; m < 16; m++)
    {
        from[m] = glm::lerp(from[m], to[m], speed);
    }
}

class CoordinateSpace
{
public:
    glm::vec2 WorldToScreen(const glm::vec2& worldPos);
    glm::vec2 ScreenToWorld(const glm::vec2& screenPos);

    glm::vec4 WorldToScreenRect(f32 x, f32 y, f32 width, f32 height)
    {
        glm::vec2 rectPos = glm::vec2(x, y);
        glm::vec2 rectSize = glm::vec2(width, height);
        glm::vec2 screenRectPos = WorldToScreen(rectPos);
        glm::vec2 screenRectSize = WorldToScreen(rectPos + rectSize) - screenRectPos;

        return glm::vec4(screenRectPos, screenRectSize);
    }

    const glm::vec2& CameraPosition() const
    {
        return mCameraPosition;
    }

    const glm::vec2& ScreenSize() const
    {
        return mScreenSize;
    }

    s32 Height() const { return mH; }
    s32 Width() const { return mW; }

    void SetScreenSize(const glm::vec2& size)
    {
        if (mScreenSize != size)
        {
            mScreenSize = size;
            ReCalculateCamera();
        }
    }

    void SetCameraPosition(const glm::vec2& pos)
    {
        if (mCameraPosition != pos)
        {
            mCameraPosition = pos;
            ReCalculateCamera();
        }
    }

    void UpdateCamera()
    {
        if (mSmoothCameraPosition)
        {
            MatrixLerp(glm::value_ptr(mView), glm::value_ptr(mTargetView), 0.2f);
            MatrixLerp(glm::value_ptr(mProjection), glm::value_ptr(mTargetProjection), 0.2f);
        }
    }

protected:
    void ReCalculateCamera()
    {
        /*
        T left, T right,
        T bottom, T top,
        T zNear, T zFar
        */
        mTargetProjection = glm::ortho(
            -mScreenSize.x / 2.0f, 
            mScreenSize.x / 2.0f, 
            mScreenSize.y / 2.0f, 
            -mScreenSize.y / 2.0f, 
            1.0f, // -1.0f
            0.0f);
        
        mTargetView = glm::translate(glm::mat4(1.0f), glm::vec3(-mCameraPosition, 0));

        if (!mSmoothCameraPosition)
        {
            mView = mTargetView;
            mProjection = mTargetProjection;
        }
    }

    glm::vec2 mScreenSize = glm::vec2(368, 240);
    glm::vec2 mCameraPosition = glm::vec2(0, 0);

public: // TODO: Only allow writing to during Update() and make read only during Render()

    bool mSmoothCameraPosition = false;

protected:

    int mW = 0;
    int mH = 0;

    // There is no "world" matrix as objects are already in world space

    glm::mat4 mTargetProjection;
    glm::mat4 mTargetView;

    // 2d ortho projection
    glm::mat4 mProjection;

    // camera location into the world
    glm::mat4 mView;
};

class AbstractRenderer : public CoordinateSpace // TODO: "Has-a" makes more sense
{
public:
    enum class eTextureFormats
    {
        eRGB,
        eRGBA,
        eA
    };

    enum eLayers
    {
        // Higher numbers render on top

        // Set at the start of each frame
        eDefaultLayer = 1000,

        // Main background image
        eForegroundMain = 2000,

        // Aka FG1 - the layer that renders on top of the "background"
        eForegroundLayer0 = 3000,

        // Aka FG1 - the layer that renders on top of the "foreground"
        eForegroundLayer1 = 4000,

        eEditor = 5000,

        eEditorUi = 6000,
        eDebugUi,

        eFmv = 7000,
    };

    enum eBlendModes
    {
        eNormal,
        eAdditive,
        eSubtractive,
        eOpaque,
        eB100F100
    };

    enum eCoordinateSystem
    {
        eScreen,
        eWorld
    };


    AbstractRenderer();
    virtual ~AbstractRenderer();

    void BeginFrame(int w, int h);
    void EndFrame();
    void Init(const char* regularFontPath, const char* italticFontPath, const char* boldFontPath);
    void ShutDown();

    virtual void ClearFrameBufferImpl(f32 r, f32 g, f32 b, f32 a) = 0;
    virtual void RenderCommandsImpl() = 0;
    virtual void ImGuiRender() = 0;

    virtual const char* Name() const = 0;
    virtual void SetVSync(bool on) = 0;

    virtual TextureHandle CreateTexture(eTextureFormats internalFormat, u32 width, u32 height, eTextureFormats inputFormat, const void *pixels, bool interpolation) = 0;
    void DestroyTexture(TextureHandle handle);

    // Drawing commands, which will be buffered and issued at the end of the frame.

    void TexturedQuad(TextureHandle texHandle, f32 x, f32 y, f32 w, f32 h, int layer, ColourU8 colour, eBlendModes blendMode = eBlendModes::eNormal, eCoordinateSystem coordinateSystem = eCoordinateSystem::eWorld);
    void Rect(f32 x, f32 y, f32 w, f32 h, int layer, ColourU8 colour, eBlendModes blendMode = eBlendModes::eNormal, eCoordinateSystem coordinateSystem = eCoordinateSystem::eWorld);
    void Text(f32 x, f32 y, f32 fontSize, const char* text, ColourU8 colour, int layer, eBlendModes blendMode = eBlendModes::eNormal, eCoordinateSystem coordinateSystem = eCoordinateSystem::eWorld);
    void PathBegin();
    void PathLineTo(f32 x, f32 y);
    void PathFill(ColourU8 colour, int layer, eBlendModes blendMode = eBlendModes::eNormal, eCoordinateSystem coordinateSystem = eCoordinateSystem::eWorld);
    void PathStroke(ColourU8 colour, f32 width, int layer, eBlendModes blendMode = eBlendModes::eNormal, eCoordinateSystem coordinateSystem = eCoordinateSystem::eWorld);
    void Line(ColourU8 colour, f32 p1x, f32 p1y, f32 p2x, f32 p2y, f32 lineWidth, int layer, eBlendModes blendMode = eBlendModes::eNormal, eCoordinateSystem coordinateSystem = eCoordinateSystem::eWorld);
    void CircleFilled(ColourU8 colour, f32 x, f32 y, f32 radius, u32 numSegments,  int layer, eBlendModes blendMode = eBlendModes::eNormal, eCoordinateSystem coordinateSystem = eCoordinateSystem::eWorld);
    void FontStashTextureDebug(f32 x, f32 y);
    void TextBounds(f32 x, f32 y, f32 fontSize, const char* text, f32* bounds);

protected:
    struct CmdState
    {
        AbstractRenderer* mThisPtr;
        eCoordinateSystem mCoordinateSystem;
        eBlendModes mBlendMode;
    };
    virtual void OnSetRenderState(CmdState& info) = 0;
private:
    enum eDrawCommands : u8
    {
        eTexturedQuad,
        eRect,
        ePath,
        ePathFill,
        ePathStroke,
        eLine,
        eCircleFilled,
        eText,
        eImGuiUi
    };

    struct CmdHeader
    {
        u32 mSize;
        u32 mLayer;
        eDrawCommands mType;
        ColourU8 mColour;
        CmdState mState;
    };

    struct CmdTexturedQuad
    {
        CmdHeader mHeader;
        TextureHandle mTexture;
        f32 mX;
        f32 mY;
        f32 mW;
        f32 mH;
    };

    struct CmdRect
    {
        CmdHeader mHeader;
        f32 mX;
        f32 mY;
        f32 mW;
        f32 mH;
    };

    struct SubCmdPathLineTo
    {
        f32 mX;
        f32 mY;
    };

    struct SubCmdPathStrokeOrFill
    {
        eDrawCommands mType;
        f32 mLineWidth;
    };

    struct CmdBeginPath
    {
        CmdHeader mHeader;
        SubCmdPathStrokeOrFill mFillOrStroke;
    };

    struct CmdLine
    {
        CmdHeader mHeader;
        f32 mP1X;
        f32 mP1Y;
        f32 mP2X;
        f32 mP2Y;
        f32 mLineWidth;
    };

    struct CmdCircleFilled
    {
        CmdHeader mHeader;
        f32 mX;
        f32 mY;
        f32 mRadius;
        u32 mNumSegments;
    };

    struct CmdText
    {
        CmdHeader mHeader;
        f32 mX;
        f32 mY;
        f32 mFontSize;
        char mText;
    };

    void EnsureCmdFreeSpace(u32 size);
    void generateImGuiCommands();
    static void RenderCallBack(const struct ImDrawList*, const ImDrawCmd* cmd);
    void PushCallBack(eCoordinateSystem& lastCoordSystem, eBlendModes& lastBlendMode, CmdHeader& header, bool force = false);
    void PushTexture(ImTextureID& last, ImTextureID current);

    std::vector<u8> mDrawCommandBuffer;
    u32 mWritePos = 0;
    bool mInPath = false;
    u32 mPathBeginPos = 0;

    // Rather than moving around lots of data to sort mDrawCommandBuffer
    // we just sort points to items in mDrawCommandBuffer instead and then iterate
    // this when generating GPU commands.
    std::vector<u8*> mPointersToOrderedCommands;

    static int FontStashRenderCreate(void* uptr, int width, int height);
    static void FontStashRenderDelete(void* uptr);
    static int FontStashRenderResize(void* uptr, int width, int height);
    static void FontStashRenderUpdate(void* uptr, int* rect, const unsigned char* data);
    static void FontStashRenderDraw(void* uptr, const float* verts, const float* tcoords, const unsigned int* colors, int nverts);

    std::unique_ptr<struct FONSparams> mFontStashParams;
    struct FONScontext* mFontStashContext = nullptr;
    TextureHandle mFontStashTexture;
    int mFontStashFontHandles[3] = {};
    u32 mFontStashWidth = 0;
    u32 mFontStashHeight = 0;

    void HandleTextCommand(f32 dx, f32 dy, f32 fontSize, const char* text, ColourU8* colour, f32* bounds);

    void AddUiCmd();
protected:
    virtual void DestroyTextures() = 0;
 
    std::vector<TextureHandle> mDestroyTextureList;
    bool mScreenSizeChanged = false;

    ImDrawData mRenderDrawData;
    ImDrawList mDrawList;
    ImVector<ImDrawList*> mRenderDrawLists;
};
