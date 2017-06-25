#include "abstractrenderer.hpp"
#include "oddlib/exceptions.hpp"

#include <algorithm>
#include <cassert>
#include "SDL.h"
#include "SDL_pixels.h"

#define FONTSTASH_IMPLEMENTATION

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wunused-result"
#endif
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4456)
#pragma warning(disable:4100)
#endif
#include "fontstash/src/fontstash.h"
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#include <boost/utility/string_view.hpp>
#include "string_util.hpp"

glm::vec2 CoordinateSpace::WorldToScreen(const glm::vec2& worldPos)
{
    return ((mProjection * mView) * glm::vec4(worldPos, 1, 1)) * glm::vec4(mW / 2, -mH / 2, 1, 1) + glm::vec4(mW / 2, mH / 2, 0, 0);
}

glm::vec2 CoordinateSpace::ScreenToWorld(const glm::vec2& screenPos)
{
    return glm::inverse(mProjection * mView) * ((glm::vec4(screenPos.x ,screenPos.y, 1, 1) - glm::vec4(mW / 2, mH / 2, 0, 0)) / glm::vec4(mW / 2, -mH / 2, 1, 1));
}

AbstractRenderer::AbstractRenderer()
{
    // These should be large enough so that no allocations are done during game
    mDestroyTextureList.reserve(1024);
    mPointersToOrderedCommands.reserve(1024*10);
    mDrawCommandBuffer.reserve(1024*1024);

    mFontStashParams = std::make_unique<FONSparams>();
    mFontStashParams->userPtr = this;
    mFontStashParams->width = 512;
    mFontStashParams->height = 512;
    mFontStashParams->flags = FONS_ZERO_TOPLEFT;
    mFontStashParams->renderCreate = FontStashRenderCreate;
    mFontStashParams->renderDelete = FontStashRenderDelete;
    mFontStashParams->renderUpdate = FontStashRenderUpdate;
    mFontStashParams->renderResize = FontStashRenderResize;
    mFontStashParams->renderDraw = FontStashRenderDraw;
}

AbstractRenderer::~AbstractRenderer()
{

}

void AbstractRenderer::Init(const char* regularFontPath, const char* italticFontPath, const char* boldFontPath)
{
    if (!mFontStashContext)
    {
        mFontStashContext = fonsCreateInternal(mFontStashParams.get());
    }

    mFontStashFontHandles[0] = fonsAddFont(mFontStashContext, "regular", regularFontPath);
    if (mFontStashFontHandles[0] == -1)
    {
        throw Oddlib::Exception(std::string("Failed to load font ") + regularFontPath);
    }

    mFontStashFontHandles[1] = fonsAddFont(mFontStashContext, "italic", italticFontPath);
    if (mFontStashFontHandles[1] == -1)
    {
        throw Oddlib::Exception(std::string("Failed to load font ") + italticFontPath);
    }

    mFontStashFontHandles[2] = fonsAddFont(mFontStashContext, "bold", boldFontPath);
    if (mFontStashFontHandles[2] == -1)
    {
        throw Oddlib::Exception(std::string("Failed to load font ") + boldFontPath);
    }
}

void AbstractRenderer::ShutDown()
{
    DestroyTexture(mFontStashTexture);
    DestroyTextures();
    if (mFontStashContext)
    {
        fonsDeleteInternal(mFontStashContext);
    }
    ImGui::Shutdown();
}

void AbstractRenderer::BeginFrame(int w, int h)
{
    assert(mDrawCommandBuffer.empty());
    assert(mPointersToOrderedCommands.empty());
    assert(mRenderDrawLists.empty());
    assert(mWritePos == 0);

    ClearFrameBufferImpl(0.4f, 0.4f, 0.4f, 1.0f);
    if (mW != w)
    {
        mW = w;
        mScreenSizeChanged = true;
    }

    if (mH != h)
    {
        mH = h;
        mScreenSizeChanged = true;
    }

    if (mScreenSizeChanged)
    {
        fonsResetAtlas(mFontStashContext, 512, 512);
    }
}

void AbstractRenderer::EndFrame()
{
    AddUiCmd();

    if (!mDrawCommandBuffer.empty())
    {
        u8* ptr = mDrawCommandBuffer.data();
        do
        {
            mPointersToOrderedCommands.push_back(ptr);
            ptr += reinterpret_cast<CmdHeader*>(ptr)->mSize;
        } while (ptr != mDrawCommandBuffer.data() + mDrawCommandBuffer.size());

        // This is the primary reason for buffering drawing command. Call order doesn't determine draw order, but layers do.
        std::stable_sort(mPointersToOrderedCommands.begin(), mPointersToOrderedCommands.end(), [](const u8* a, const u8* b)
        {
            return (*reinterpret_cast<const CmdHeader*>(a)).mLayer < (*reinterpret_cast<const CmdHeader*>(b)).mLayer;
        });

        generateImGuiCommands();
    }

    RenderCommandsImpl();

    DestroyTextures();
    mScreenSizeChanged = false;

    mWritePos = 0;
    mDrawList.Clear();
    mDrawCommandBuffer.clear();
    mPointersToOrderedCommands.clear();
    mRenderDrawLists.clear();
    mRenderDrawData.CmdListsCount = 0;
}

void AbstractRenderer::DestroyTexture(TextureHandle handle)
{
    if (handle.IsValid())
    {
        mDestroyTextureList.push_back(handle); // Delay the deletion after drawing current frame
    }
}

static u32 ToImCol(const ColourU8& col)
{
    return IM_COL32(col.r, col.g, col.b, col.a);
}

void AbstractRenderer::RenderCallBack(const struct ImDrawList*, const ImDrawCmd* cmd)
{
    CmdHeader* data = reinterpret_cast<CmdHeader*>(cmd->UserCallbackData);
    data->mState.mThisPtr->OnSetRenderState(data->mState);
    if (data->mType == eImGuiUi)
    {
        data->mState.mThisPtr->ImGuiRender();
    }
}

void AbstractRenderer::PushCallBack(AbstractRenderer::eCoordinateSystem& lastCoordSystem, AbstractRenderer::eBlendModes& lastBlendMode, AbstractRenderer::CmdHeader& header, bool force)
{
    if (force || (header.mState.mCoordinateSystem != lastCoordSystem || header.mState.mBlendMode != lastBlendMode))
    {
        header.mState.mThisPtr = this;
        mDrawList.AddCallback(RenderCallBack, &header);
        lastBlendMode = header.mState.mBlendMode;
        lastCoordSystem = header.mState.mCoordinateSystem;
    }
}

void AbstractRenderer::PushTexture(ImTextureID& last, ImTextureID current)
{
    if (last != current)
    {
        mDrawList.PushTextureID(current);
        last = current;
    }
}

/*static*/ int AbstractRenderer::FontStashRenderCreate(void* uptr, int width, int height)
{
    auto pRenderer = reinterpret_cast<AbstractRenderer*>(uptr);
    pRenderer->mFontStashWidth = width;
    pRenderer->mFontStashHeight = height;
    return 1;
}

void AbstractRenderer::FontStashRenderDelete(void* uptr)
{
    auto pRenderer = reinterpret_cast<AbstractRenderer*>(uptr);
    pRenderer->DestroyTexture(pRenderer->mFontStashTexture);
}

int AbstractRenderer::FontStashRenderResize(void* uptr, int width, int height)
{
    TRACE_ENTRYEXIT;
    auto pRenderer = reinterpret_cast<AbstractRenderer*>(uptr);
    pRenderer->mFontStashWidth = width;
    pRenderer->mFontStashHeight = height;
    return 1;
}

void AbstractRenderer::FontStashRenderUpdate(void* uptr, int* /*rect*/, const unsigned char* data)
{
    auto pRenderer = reinterpret_cast<AbstractRenderer*>(uptr);
    FontStashRenderDelete(uptr);
    pRenderer->mFontStashTexture = pRenderer->CreateTexture(eTextureFormats::eRGBA, pRenderer->mFontStashWidth, pRenderer->mFontStashHeight, eTextureFormats::eA, data, true);
}

static void PrimTriangleUV(
    ImDrawList& list,
    const ImVec2& a,
    const ImVec2& b,
    const ImVec2& c,
    const ImVec2& uv_a,
    const ImVec2& uv_b,
    const ImVec2& uv_c,
    ImU32 col)
{
    ImDrawIdx idx = (ImDrawIdx)list._VtxCurrentIdx;
    list._IdxWritePtr[0] = idx; 
    list._IdxWritePtr[1] = (ImDrawIdx)(idx + 1); 
    list._IdxWritePtr[2] = (ImDrawIdx)(idx + 2);

    list._VtxWritePtr[0].pos = a; list._VtxWritePtr[0].uv = uv_a; list._VtxWritePtr[0].col = col;
    list._VtxWritePtr[1].pos = b; list._VtxWritePtr[1].uv = uv_b; list._VtxWritePtr[1].col = col;
    list._VtxWritePtr[2].pos = c; list._VtxWritePtr[2].uv = uv_c; list._VtxWritePtr[2].col = col;

    list._VtxWritePtr += 3;
    list._VtxCurrentIdx += 3;
    list._IdxWritePtr += 3;
}

/*
verts pointer to vertex position data, 2 floats per vertex
tcoords pointer to texture coordinate data, 2 floats per vertex
colors pointer to color data, 1 uint per vertex (or 4 bytes)
nverts is the number of vertices to draw
*/
void AbstractRenderer::FontStashRenderDraw(void* uptr, const float* verts, const float* tcoords, const unsigned int* colors, int nverts)
{
    auto pRenderer = reinterpret_cast<AbstractRenderer*>(uptr);

    const int numTris = nverts / 3;
    if (numTris)
    {
        pRenderer->mDrawList.PushTextureID(pRenderer->mFontStashTexture.mData);
        int i = 0;
        for (int j = 0; j < numTris; j++)
        {
            pRenderer->mDrawList.PrimReserve(3, 3);
            PrimTriangleUV(pRenderer->mDrawList,
                { verts[0 + i], verts[1 + i] }, 
                { verts[2 + i], verts[3 + i], },
                { verts[4 + i], verts[5 + i] },
                { tcoords[0 + i], tcoords[1 + i] },
                { tcoords[2 + i], tcoords[3 + i] },
                { tcoords[4 + i], tcoords[5 + i] },
                colors[0]);
            i += 6;
        }
    }
}


void AbstractRenderer::HandleTextCommand(f32 dx, f32 dy, f32 fontSize, const char* text, ColourU8* colour, f32* bounds)
{

    fonsSetAlign(mFontStashContext, FONS_ALIGN_LEFT | FONS_ALIGN_TOP);
    fonsSetFont(mFontStashContext, mFontStashFontHandles[0]);

    boost::string_view sv(text);
    
    const char* end = text + sv.length();

    // TODO: Change utils to use boost::string_view to avoid per frame allocs
    std::string s = sv.to_string();

    int fontToUse = 0;
    if (string_util::starts_with(s, "<i>", true) && string_util::ends_with(s, "</i>", true))
    {
        fontToUse = 1;
        text += 3;
        end -= 4;
    }
    else if (string_util::starts_with(s, "<b>", true) && string_util::ends_with(s, "</b>", true))
    {
        fontToUse = 2;
        text += 3;
        end -= 4;
    }
    
    fonsSetFont(mFontStashContext, fontToUse);
    fonsSetSize(mFontStashContext, fontSize);
    fonsSetSpacing(mFontStashContext, 0.0f);

    // Measure only
    if (bounds)
    {
        fonsTextBounds(mFontStashContext, dx, dy, text, end, bounds);
    }
    else
    {
        fonsSetBlur(mFontStashContext, 2.0f);
        fonsSetColor(mFontStashContext, ColourU8{ 0, 0, 0, 255 }.To32Bit());
        fonsDrawText(mFontStashContext, dx - 2.0f, dy + 2.0f, text, end);

        fonsSetBlur(mFontStashContext, 0.0f);
        fonsSetColor(mFontStashContext, colour->To32Bit());
        fonsDrawText(mFontStashContext, dx, dy, text, end);
    }
}

void AbstractRenderer::TextBounds(f32 x, f32 y, f32 fontSize, const char* text, f32* bounds)
{
    assert(bounds != nullptr);
    HandleTextCommand(x, y, fontSize, text, nullptr, bounds);
}

void AbstractRenderer::generateImGuiCommands()
{
    // Used to cache previous state and skip redundant ones
    eCoordinateSystem lastCoordSystem = eCoordinateSystem::eScreen;
    eBlendModes lastBlendMode = eBlendModes::eOpaque;
    ImTextureID lastTextureId = nullptr;

    for (u8* cmdType : mPointersToOrderedCommands)
    {
        switch (reinterpret_cast<CmdHeader*>(cmdType)->mType)
        {
        case eImGuiUi:
        {
            CmdHeader* cmd = reinterpret_cast<CmdHeader*>(cmdType);
            PushCallBack(lastCoordSystem, lastBlendMode, *cmd, true);
        }
        break;

        case eRect:
        {
            CmdRect* cmd = reinterpret_cast<CmdRect*>(cmdType);
            PushCallBack(lastCoordSystem, lastBlendMode, cmd->mHeader);
            PushTexture(lastTextureId, ImGui::GetIO().Fonts->TexID);
            mDrawList.AddRect({ cmd->mX, cmd->mY }, { cmd->mX+cmd->mW, cmd->mY+cmd->mH }, ToImCol(cmd->mHeader.mColour));
        }
        break;

        case eTexturedQuad:
        {
            CmdTexturedQuad* cmd = reinterpret_cast<CmdTexturedQuad*>(cmdType);
            PushCallBack(lastCoordSystem, lastBlendMode, cmd->mHeader);
            PushTexture(lastTextureId, cmd->mTexture.mData);
            mDrawList.PrimReserve(6, 4);
            mDrawList.PrimRectUV(
                { cmd->mX, cmd->mY },
                { cmd->mX + cmd->mW, cmd->mY + cmd->mH },
                { 0.0f, 0.0f },
                { 1.0f, 1.0f },
                ToImCol(cmd->mHeader.mColour));
        }
        break;

        case eText:
        {
            CmdText* cmd = reinterpret_cast<CmdText*>(cmdType);
            PushCallBack(lastCoordSystem, lastBlendMode, cmd->mHeader, true);
            PushTexture(lastTextureId, ImGui::GetIO().Fonts->TexID);
            mDrawList.PushClipRectFullScreen();
            HandleTextCommand(cmd->mX, cmd->mY, cmd->mFontSize, &cmd->mText, &cmd->mHeader.mColour, nullptr);
        }
        break;

        case ePath:
        {
            CmdBeginPath* cmd = reinterpret_cast<CmdBeginPath*>(cmdType);
            PushCallBack(lastCoordSystem, lastBlendMode, cmd->mHeader);
            PushTexture(lastTextureId, ImGui::GetIO().Fonts->TexID);

            const u32 remainderSize = cmd->mHeader.mSize - sizeof(CmdBeginPath);
            const u32 numSubCmds = remainderSize / sizeof(SubCmdPathLineTo);
            SubCmdPathLineTo* subCmd = reinterpret_cast<SubCmdPathLineTo*>(cmdType += sizeof(CmdBeginPath));
            for (u32 i = 0; i < numSubCmds; i++)
            {
                mDrawList.PathLineTo({ subCmd->mX, subCmd->mY });
                subCmd++;
            }

            if (cmd->mFillOrStroke.mType == ePathFill)
            {
                mDrawList.PathFillConvex(ToImCol(cmd->mHeader.mColour));
            }
            else if (cmd->mFillOrStroke.mType == ePathStroke)
            {
                mDrawList.PathStroke(ToImCol(cmd->mHeader.mColour), true, cmd->mFillOrStroke.mLineWidth);
            }
            else
            {
                assert(false);
            }
        }
        break;

        case eLine:
        {
            CmdLine* cmd = reinterpret_cast<CmdLine*>(cmdType);
            PushCallBack(lastCoordSystem, lastBlendMode, cmd->mHeader);
            PushTexture(lastTextureId, ImGui::GetIO().Fonts->TexID);
            mDrawList.AddLine({ cmd->mP1X, cmd->mP1Y }, { cmd->mP2X, cmd->mP2Y }, ToImCol(cmd->mHeader.mColour), cmd->mLineWidth);
        }
        break;

        case eCircleFilled:
        {
            CmdCircleFilled* cmd = reinterpret_cast<CmdCircleFilled*>(cmdType);
            PushCallBack(lastCoordSystem, lastBlendMode, cmd->mHeader);
            PushTexture(lastTextureId, ImGui::GetIO().Fonts->TexID);
            mDrawList.AddCircleFilled({ cmd->mX, cmd->mY }, cmd->mRadius, ToImCol(cmd->mHeader.mColour), cmd->mNumSegments);
        }
        break;

        default:
            assert(false);
        }
    }

    mRenderDrawLists.push_back(&mDrawList);
    mRenderDrawData.CmdListsCount = mRenderDrawLists.Size;
    mRenderDrawData.CmdLists = mRenderDrawLists.Data;
    mRenderDrawData.TotalIdxCount = 0;
    mRenderDrawData.TotalVtxCount = 0;
    for (auto& list : mRenderDrawLists)
    {
        mRenderDrawData.TotalIdxCount += list->IdxBuffer.size();
        mRenderDrawData.TotalVtxCount += list->VtxBuffer.size();
    }
}

void AbstractRenderer::TexturedQuad(TextureHandle texHandle, f32 x, f32 y, f32 w, f32 h, int layer, ColourU8 colour, eBlendModes blendMode, eCoordinateSystem coordinateSystem)
{
    assert(mInPath == false);
    EnsureCmdFreeSpace(sizeof(CmdTexturedQuad));
    u8* const ptr = mDrawCommandBuffer.data() + mWritePos;
    mWritePos += sizeof(CmdTexturedQuad);
    CmdTexturedQuad* const cmd = new (ptr) CmdTexturedQuad;
    cmd->mHeader.mType = eTexturedQuad;
    cmd->mHeader.mSize = sizeof(CmdTexturedQuad);
    cmd->mHeader.mColour = colour;
    cmd->mHeader.mLayer = layer;
    cmd->mX = x;
    cmd->mY = y;
    cmd->mW = w;
    cmd->mH = h;
    cmd->mTexture = texHandle;
    cmd->mHeader.mState.mBlendMode = blendMode;
    cmd->mHeader.mState.mCoordinateSystem = coordinateSystem;
}

void AbstractRenderer::Rect(f32 x, f32 y, f32 w, f32 h, int layer, ColourU8 colour, eBlendModes blendMode, eCoordinateSystem coordinateSystem)
{
    assert(mInPath == false);
    EnsureCmdFreeSpace(sizeof(CmdRect));
    u8* const ptr = mDrawCommandBuffer.data() + mWritePos;
    mWritePos += sizeof(CmdRect);
    CmdRect* const cmd = new (ptr) CmdRect;
    cmd->mHeader.mType = eRect;
    cmd->mHeader.mSize = sizeof(CmdRect);
    cmd->mHeader.mColour = colour;
    cmd->mHeader.mLayer = layer;
    cmd->mX = x;
    cmd->mY = y;
    cmd->mW = w;
    cmd->mH = h;
    cmd->mHeader.mState.mBlendMode = blendMode;
    cmd->mHeader.mState.mCoordinateSystem = coordinateSystem;
}

void AbstractRenderer::Text(f32 x, f32 y, f32 fontSize, const char* text, ColourU8 colour, int layer, eBlendModes blendMode, eCoordinateSystem coordinateSystem)
{
    assert(mInPath == false);
    const u32 textLength = static_cast<u32>(strlen(text)+1);
    EnsureCmdFreeSpace(sizeof(CmdText) + textLength);
    u8* const ptr = mDrawCommandBuffer.data() + mWritePos;
    mWritePos += sizeof(CmdText) + textLength;
    CmdText* const cmd = new (ptr) CmdText;
    cmd->mHeader.mType = eText;
    cmd->mHeader.mSize = sizeof(CmdText) + textLength;
    cmd->mHeader.mColour = colour;
    cmd->mHeader.mLayer = layer;
    cmd->mX = x;
    cmd->mY = y;
    cmd->mFontSize = fontSize;
    memcpy(&cmd->mText, text, textLength);
    cmd->mHeader.mState.mBlendMode = blendMode;
    cmd->mHeader.mState.mCoordinateSystem = coordinateSystem;
}

void AbstractRenderer::PathBegin()
{
    assert(mInPath == false);
    mInPath = true;
    mPathBeginPos = mWritePos;
    EnsureCmdFreeSpace(sizeof(CmdBeginPath));
    u8* const ptr = mDrawCommandBuffer.data() + mWritePos;
    mWritePos += sizeof(CmdBeginPath);
    CmdBeginPath* const cmd = new (ptr) CmdBeginPath;
    cmd->mHeader.mType = ePath;
}

void AbstractRenderer::PathLineTo(f32 x, f32 y)
{
    assert(mInPath == true);
    EnsureCmdFreeSpace(sizeof(SubCmdPathLineTo));
    u8* const ptr = mDrawCommandBuffer.data() + mWritePos;
    mWritePos += sizeof(SubCmdPathLineTo);
    SubCmdPathLineTo* const cmd = new (ptr) SubCmdPathLineTo;
    cmd->mX = x;
    cmd->mY = y;
}

void AbstractRenderer::PathFill(ColourU8 colour, int layer, eBlendModes blendMode, eCoordinateSystem coordinateSystem)
{
    assert(mInPath == true);

    u32 cmdSize = mWritePos - mPathBeginPos;
    CmdBeginPath* cmdPathBegin = reinterpret_cast<CmdBeginPath*>(mDrawCommandBuffer.data() + mPathBeginPos);
    cmdPathBegin->mHeader.mSize = cmdSize;
    cmdPathBegin->mHeader.mLayer = layer;
    cmdPathBegin->mHeader.mColour = colour;
    cmdPathBegin->mHeader.mState.mBlendMode = blendMode;
    cmdPathBegin->mHeader.mState.mCoordinateSystem = coordinateSystem;
    cmdPathBegin->mFillOrStroke.mType = ePathFill;

    mInPath = false;
}

void AbstractRenderer::PathStroke(ColourU8 colour, f32 width, int layer, eBlendModes blendMode, eCoordinateSystem coordinateSystem)
{
    assert(mInPath == true);

    u32 cmdSize = mWritePos - mPathBeginPos;
    CmdBeginPath* cmdPathBegin = reinterpret_cast<CmdBeginPath*>(mDrawCommandBuffer.data() + mPathBeginPos);
    cmdPathBegin->mHeader.mSize = cmdSize;
    cmdPathBegin->mHeader.mLayer = layer;
    cmdPathBegin->mHeader.mColour = colour;
    cmdPathBegin->mHeader.mState.mBlendMode = blendMode;
    cmdPathBegin->mHeader.mState.mCoordinateSystem = coordinateSystem;
    cmdPathBegin->mFillOrStroke.mType = ePathStroke;
    cmdPathBegin->mFillOrStroke.mLineWidth = width;

    mInPath = false;
}

void AbstractRenderer::Line(ColourU8 colour, f32 p1x, f32 p1y, f32 p2x, f32 p2y, f32 lineWidth, int layer, eBlendModes blendMode, eCoordinateSystem coordinateSystem)
{
    assert(mInPath == false);
    EnsureCmdFreeSpace(sizeof(CmdLine));
    u8* const ptr = mDrawCommandBuffer.data() + mWritePos;
    mWritePos += sizeof(CmdLine);
    CmdLine* const cmd = new (ptr) CmdLine;
    cmd->mHeader.mType = eLine;
    cmd->mHeader.mSize = sizeof(CmdLine);
    cmd->mHeader.mColour = colour;
    cmd->mHeader.mLayer = layer;
    cmd->mLineWidth = lineWidth;
    cmd->mP1X = p1x;
    cmd->mP1Y = p1y;
    cmd->mP2X = p2x;
    cmd->mP2Y = p2y;
    cmd->mHeader.mState.mBlendMode = blendMode;
    cmd->mHeader.mState.mCoordinateSystem = coordinateSystem;
}

void AbstractRenderer::CircleFilled(ColourU8 colour, f32 x, f32 y, f32 radius, u32 numSegments, int layer, eBlendModes blendMode, eCoordinateSystem coordinateSystem)
{
    assert(mInPath == false);
    EnsureCmdFreeSpace(sizeof(CmdCircleFilled));
    u8* const ptr = mDrawCommandBuffer.data() + mWritePos;
    mWritePos += sizeof(CmdCircleFilled);
    CmdCircleFilled* const cmd = new (ptr) CmdCircleFilled;
    cmd->mHeader.mType = eCircleFilled;
    cmd->mHeader.mSize = sizeof(CmdCircleFilled);
    cmd->mHeader.mColour = colour;
    cmd->mHeader.mLayer = layer;
    cmd->mRadius = radius;
    cmd->mX = x;
    cmd->mY = y;
    cmd->mNumSegments = numSegments;
    cmd->mHeader.mState.mBlendMode = blendMode;
    cmd->mHeader.mState.mCoordinateSystem = coordinateSystem;
}

void AbstractRenderer::AddUiCmd()
{
    assert(mInPath == false);
    EnsureCmdFreeSpace(sizeof(CmdHeader));
    u8* const ptr = mDrawCommandBuffer.data() + mWritePos;
    mWritePos += sizeof(CmdHeader);
    CmdHeader* const cmd = new (ptr) CmdHeader;
    cmd->mType = eImGuiUi;
    cmd->mLayer = eFmv;
    cmd->mSize = sizeof(CmdHeader);
    cmd->mState.mThisPtr = this;
    cmd->mState.mBlendMode = eNormal;
    cmd->mState.mCoordinateSystem = eScreen;
}

void AbstractRenderer::FontStashTextureDebug(f32 x, f32 y)
{
    TexturedQuad(mFontStashTexture,
        x, y, static_cast<f32>(mFontStashWidth), static_cast<f32>(mFontStashHeight),
        eFmv+2, { 255,255,255,255 }, eBlendModes::eNormal, eScreen);
}

void AbstractRenderer::EnsureCmdFreeSpace(u32 size)
{
    if (mDrawCommandBuffer.size() - mWritePos < size)
    {
        mDrawCommandBuffer.resize(mDrawCommandBuffer.size() + size);
    }
}
