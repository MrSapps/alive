#include "directx9renderer.hpp"

#include <windows.h>
#include "SDL_syswm.h"
#include "oddlib/exceptions.hpp"
#include <algorithm>
#include "logger.hpp"

#pragma comment(lib, "d3d9.lib")

void DirectX9Renderer::PopulatePresentParams(bool vSync)
{
    ZeroMemory(&mPresentParameters, sizeof(mPresentParameters));
    mPresentParameters.Windowed = true;
    mPresentParameters.SwapEffect = D3DSWAPEFFECT_FLIP;
    mPresentParameters.EnableAutoDepthStencil = true;
    mPresentParameters.AutoDepthStencilFormat = D3DFMT_D16;
    mPresentParameters.hDeviceWindow = mHwnd;
    mPresentParameters.BackBufferWidth = 0; // Because Windowed = true this will use the current window size
    mPresentParameters.BackBufferHeight = 0; // Because Windowed = true this will use the current window size
    mPresentParameters.BackBufferFormat = D3DFMT_UNKNOWN; // Because Windowed = true this will use the current format
    mPresentParameters.MultiSampleType = D3DMULTISAMPLE_NONE;

    if (vSync)
    {
        mPresentParameters.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
        mPresentParameters.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
    }
    else
    {
        mPresentParameters.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
        mPresentParameters.FullScreen_RefreshRateInHz = 0;
    }
}

void DirectX9Renderer::GetHWND(SDL_Window* window)
{
    SDL_SysWMinfo systemInfo = {};
    SDL_VERSION(&systemInfo.version);
    SDL_GetWindowWMInfo(window, &systemInfo);

    mHwnd = systemInfo.info.win.window;
}

DirectX9Renderer::DirectX9Renderer(SDL_Window* window)
{
    TRACE_ENTRYEXIT;

    GetHWND(window);

    mD3D9.Attach(Direct3DCreate9(D3D_SDK_VERSION));
    if (mD3D9 == nullptr)
    {
        throw Oddlib::Exception("Could not create Direct3D 9 Object");
    }

    PopulatePresentParams(true);
    
    if (FAILED(mD3D9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
        mHwnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING,
        &mPresentParameters, &mDevice)))
    {
        throw Oddlib::Exception("Could not create Direct3D 9 Device");
    }

    SetRendererStates();

    mDevice->SetFVF(D3DVERTEX::kFVF);
}

void DirectX9Renderer::SetRendererStates()
{
    mDevice->SetRenderState(D3DRS_AMBIENT, RGB(255, 255, 255));

    mDevice->SetRenderState(D3DRS_LIGHTING, false);

    // Turn the Z Buffer off
    mDevice->SetRenderState(D3DRS_ZENABLE, FALSE);

    // Turn alpha blending off
    mDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);

    // Alpha params (where 1.0f alpha = transparent)
    mDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    mDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    mDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);

    mDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

    mDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
}

void DirectX9Renderer::SetWorldMatrix()
{
    D3DMATRIX orthographicMatrix;
    glm::mat4 projectionTransposed = glm::transpose(mProjection);
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            orthographicMatrix.m[i][j] = projectionTransposed[j][i];
        }
    }

    mDevice->SetTransform(D3DTS_PROJECTION, &orthographicMatrix);

    D3DMATRIX viewMatrix;
    glm::mat4 viewTransposed = glm::transpose(mView);
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            viewMatrix.m[i][j] = viewTransposed[j][i];
        }
    }

    mDevice->SetTransform(D3DTS_VIEW, &viewMatrix);
}

DirectX9Renderer::~DirectX9Renderer()
{
    TRACE_ENTRYEXIT;
}

void DirectX9Renderer::SetVSync(bool on)
{
    TRACE_ENTRYEXIT;
    mVsyncEnabled = on;
    mScreenSizeChanged = true;
}

void DirectX9Renderer::ClearFrameBufferImpl(f32 r, f32 g, f32 b, f32 a)
{
    SetRendererStates();

    mDevice->BeginScene();
    mDevice->Clear(0, NULL, D3DCLEAR_TARGET /*| D3DCLEAR_ZBUFFER*/, D3DCOLOR_COLORVALUE(r, g, b, a), 1.0f, 0);
}

static inline TextureHandle DxToTextureHandle(LPDIRECT3DTEXTURE9 pTexture)
{
    TextureHandle ret;
    ret.mData = reinterpret_cast<void*>(pTexture);
    return ret;
}

static inline LPDIRECT3DTEXTURE9 TextureHandleToDx(TextureHandle handle)
{
    return reinterpret_cast<LPDIRECT3DTEXTURE9>(handle.mData);
}

template<class T>
class BufferUnLocker
{
public:
    BufferUnLocker(T& buffer)
        : mBuffer(buffer)
    {

    }

    ~BufferUnLocker()
    {
        const HRESULT hr = mBuffer->Unlock();
        if (FAILED(hr))
        {
            LOG_ERROR("Failed to unlock buffer: " << hr);
        }
    }

private:
    T& mBuffer;
};

void DirectX9Renderer::ImGuiRender(struct ImDrawData* data, Microsoft::WRL::ComPtr<IDirect3DVertexBuffer9>& vbo, int& vboSize, Microsoft::WRL::ComPtr<IDirect3DIndexBuffer9>& ibo, int& iboSize)
{
    ImGuiIO& io = ImGui::GetIO();

    // Create and grow buffers if needed
    if (!vbo || vboSize < data->TotalVtxCount)
    {
        vbo.Reset();
        vboSize = data->TotalVtxCount + 5000;
        if (FAILED(mDevice->CreateVertexBuffer(vboSize * sizeof(D3DVERTEX), D3DUSAGE_WRITEONLY, D3DVERTEX::kFVF, D3DPOOL_MANAGED, &vbo, NULL)))
        {
            return;
        }
    }

    if (!ibo || iboSize < data->TotalIdxCount)
    {
        ibo.Reset();
        iboSize = data->TotalIdxCount + 10000;
        if (FAILED(mDevice->CreateIndexBuffer(iboSize * sizeof(ImDrawIdx), D3DUSAGE_WRITEONLY, sizeof(ImDrawIdx) == 2 ? D3DFMT_INDEX16 : D3DFMT_INDEX32, D3DPOOL_MANAGED, &ibo, NULL)))
        {
            return;
        }
    }

    // Backup the DX9 state
    Microsoft::WRL::ComPtr<IDirect3DStateBlock9> d3d9_state_block;
    if (FAILED(mDevice->CreateStateBlock(D3DSBT_ALL, &d3d9_state_block)))
    {
        return;
    }

    // Copy and convert all vertices into a single contiguous buffer
    D3DVERTEX* vtx_dst;
    ImDrawIdx* idx_dst;
    {
        if (FAILED(vbo->Lock(0, (UINT)(data->TotalVtxCount * sizeof(D3DVERTEX)), (void**)&vtx_dst, 0)))
        {
            return;
        }
        BufferUnLocker<decltype(vbo)> vtx(vbo);

        if (FAILED(ibo->Lock(0, (UINT)(data->TotalIdxCount * sizeof(ImDrawIdx)), (void**)&idx_dst, 0)))
        {
            return;
        }
        BufferUnLocker<decltype(ibo)> idx(ibo);

        for (int n = 0; n < data->CmdListsCount; n++)
        {
            const ImDrawList* cmd_list = data->CmdLists[n];
            const ImDrawVert* vtx_src = cmd_list->VtxBuffer.Data;
            for (int i = 0; i < cmd_list->VtxBuffer.Size; i++)
            {
                vtx_dst->x = vtx_src->pos.x;
                vtx_dst->y = vtx_src->pos.y;
                vtx_dst->z = 0.0f;
                vtx_dst->colour = (vtx_src->col & 0xFF00FF00) | ((vtx_src->col & 0xFF0000) >> 16) | ((vtx_src->col & 0xFF) << 16);     // RGBA --> ARGB for DirectX9
                vtx_dst->u = vtx_src->uv.x;
                vtx_dst->v = vtx_src->uv.y;
                vtx_dst++;
                vtx_src++;
            }
            memcpy(idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
            idx_dst += cmd_list->IdxBuffer.Size;
        }
    }

    mDevice->SetStreamSource(0, vbo.Get(), 0, sizeof(D3DVERTEX));
    mDevice->SetIndices(ibo.Get());
    mDevice->SetFVF(D3DVERTEX::kFVF);

    // Setup viewport
    D3DVIEWPORT9 vp;
    vp.X = vp.Y = 0;
    vp.Width = (DWORD)io.DisplaySize.x;
    vp.Height = (DWORD)io.DisplaySize.y;
    vp.MinZ = 0.0f;
    vp.MaxZ = 1.0f;
    //mDevice->SetViewport(&vp);

    // Setup render state: fixed-pipeline, alpha-blending, no face culling, no depth testing
    mDevice->SetPixelShader(NULL);
    mDevice->SetVertexShader(NULL);
    mDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    mDevice->SetRenderState(D3DRS_LIGHTING, false);
    mDevice->SetRenderState(D3DRS_ZENABLE, false);
    mDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
    mDevice->SetRenderState(D3DRS_ALPHATESTENABLE, false);
    mDevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
    mDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    mDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    mDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, true);
    mDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    mDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    mDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    mDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
    mDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    mDevice->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
    mDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    mDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);

    // Render command lists
    int vtx_offset = 0;
    int idx_offset = 0;
    for (int n = 0; n < data->CmdListsCount; n++)
    {
        ImDrawList* cmd_list = data->CmdLists[n];
        doDraw(cmd_list, vtx_offset, idx_offset);
        vtx_offset += cmd_list->VtxBuffer.Size;
    }

    // Restore the DX9 state
    d3d9_state_block->Apply();
}

void DirectX9Renderer::OnSetRenderState(AbstractRenderer::CmdState& info)
{
    if (info.mCoordinateSystem == AbstractRenderer::eScreen)
    {
        SetScreenMatrix();

    }
    else if (info.mCoordinateSystem == AbstractRenderer::eWorld)
    {
        SetWorldMatrix();
    }

    if (info.mBlendMode == AbstractRenderer::eNormal)
    {
        // TODO: Handle setting blend modes
    }
}

void DirectX9Renderer::SetScreenMatrix()
{
    ImGuiIO& io = ImGui::GetIO();
    const float L = 0.5f, R = io.DisplaySize.x + 0.5f, T = 0.5f, B = io.DisplaySize.y + 0.5f;
    D3DMATRIX mat_identity = { { 1.0f, 0.0f, 0.0f, 0.0f,  0.0f, 1.0f, 0.0f, 0.0f,  0.0f, 0.0f, 1.0f, 0.0f,  0.0f, 0.0f, 0.0f, 1.0f } };
    D3DMATRIX mat_projection =
    {
        2.0f / (R - L),   0.0f,         0.0f,  0.0f,
        0.0f,         2.0f / (T - B),   0.0f,  0.0f,
        0.0f,         0.0f,         0.5f,  0.0f,
        (L + R) / (L - R),  (T + B) / (B - T),  0.5f,  1.0f,
    };
    mDevice->SetTransform(D3DTS_WORLD, &mat_identity);
    mDevice->SetTransform(D3DTS_VIEW, &mat_identity);
    mDevice->SetTransform(D3DTS_PROJECTION, &mat_projection);
}

void DirectX9Renderer::doDraw(struct ImDrawList* cmd_list, int& vtx_offset, int& idx_offset)
{
    SetScreenMatrix();

    for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
    {
        const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
        if (pcmd->UserCallback)
        {
            pcmd->UserCallback(cmd_list, pcmd);
            continue;
        }

        RECT r = { (LONG)pcmd->ClipRect.x, (LONG)pcmd->ClipRect.y, (LONG)pcmd->ClipRect.z, (LONG)pcmd->ClipRect.w };

        // TextureHandle hFont = (const TextureHandle)(pcmd->TextureId); // TODO: Should probably go via this type to be a bit safer
        // direct cast might blow up
        LPDIRECT3DTEXTURE9 pTexture = (LPDIRECT3DTEXTURE9)pcmd->TextureId;

        mDevice->SetTexture(0, pTexture);

        if (r.left < 0)
        {
            r.left = 0;
        }

        if (r.right > (LONG)mPresentParameters.BackBufferWidth)
        {
            r.right = (LONG)mPresentParameters.BackBufferWidth;
        }

        if (r.top < 0)
        {
            r.top = 0;
        }

        if (r.bottom > (LONG)mPresentParameters.BackBufferHeight)
        {
            r.bottom = (LONG)mPresentParameters.BackBufferHeight;
        }

        mDevice->SetScissorRect(&r);

        if (pcmd->ElemCount > 0)
        {
            mDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, vtx_offset, 0, (UINT)cmd_list->VtxBuffer.Size, idx_offset, pcmd->ElemCount / 3);
            mDevice->SetTexture(0, nullptr);
        }

        idx_offset += pcmd->ElemCount;
    }
}

void DirectX9Renderer::ImGuiRender()
{
    ImDrawData* data = ImGui::GetDrawData();
    if (data)
    {
        ImGuiRender(data, mGuiVBO, mGuiVertexBufferSize, mGuiIBO, mGuiIndexBufferSize);
    }
}

void DirectX9Renderer::RenderCommandsImpl()
{
    if (mRenderDrawLists.empty() == false)
    {
        ImGuiRender(&mRenderDrawData, mRendererVBO, mRendererVertexBufferSize, mRendererIBO, mRendererIndexBufferSize);
    }

    mDevice->EndScene();

    HRESULT hr = mDevice->Present(NULL, NULL, NULL, NULL);
    if (mScreenSizeChanged || hr == D3DERR_DEVICELOST || hr == D3DERR_DEVICEHUNG || hr == D3DERR_DEVICEREMOVED)
    {
        PopulatePresentParams(mVsyncEnabled);
        hr = mDevice->Reset(&mPresentParameters);
        if (FAILED(hr))
        {
            throw Oddlib::Exception("Direct 3D 9 device reset failure " + std::to_string(hr));
        }
    }
    else if (hr == S_PRESENT_OCCLUDED)
    {
        // window is not visible, so vsync won't work. Let's sleep a bit to reduce CPU usage
        Sleep(1);
    }
}

static inline D3DFORMAT ToD3DFormat(AbstractRenderer::eTextureFormats format)
{
    switch (format)
    {
    case AbstractRenderer::eTextureFormats::eRGB:
        return D3DFMT_X8R8G8B8;
    case AbstractRenderer::eTextureFormats::eRGBA:
        return D3DFMT_A8R8G8B8;
    case AbstractRenderer::eTextureFormats::eA:
        return D3DFMT_A8;
    }
    abort();
}


TextureHandle DirectX9Renderer::CreateTexture(AbstractRenderer::eTextureFormats internalFormat, u32 width, u32 height, AbstractRenderer::eTextureFormats inputFormat, const void* pixels, bool /*interpolation*/)
{
    LPDIRECT3DTEXTURE9 pTexture = nullptr;
    if (FAILED(mDevice->CreateTexture(width, height, 1, 0, ToD3DFormat(internalFormat), D3DPOOL_MANAGED, &pTexture, nullptr)))
    {
        LOG_ERROR("Create texture failed");
        return DxToTextureHandle(nullptr);
    }

    if (pixels)
    {
        D3DLOCKED_RECT lockedRect = {};
        if (FAILED(pTexture->LockRect(0, &lockedRect, nullptr, 0)))
        {
            LOG_ERROR("LockRect for texture failed");
            return DxToTextureHandle(nullptr);
        }


        DWORD* imageData = (DWORD*)lockedRect.pBits;
        BYTE* iPixelData = (BYTE*)pixels;

        DWORD srcIdx = 0;

        for (u32 y = 0; y < height; ++y)
        {
            for (u32 x = 0; x < width; x++)
            {
                unsigned char r = 0xff; 
                unsigned char g = 0xff;
                unsigned char b = 0xff;
                unsigned char a = 0xff;

                if (inputFormat == AbstractRenderer::eTextureFormats::eRGBA || inputFormat == AbstractRenderer::eTextureFormats::eRGB)
                {
                    r = iPixelData[srcIdx++];
                    g = iPixelData[srcIdx++];
                    b = iPixelData[srcIdx++];
                }

                if (inputFormat == AbstractRenderer::eTextureFormats::eRGBA || inputFormat == AbstractRenderer::eTextureFormats::eA)
                {
                    a = iPixelData[srcIdx++];
                }

                const DWORD index = (x * 4 + (y*(lockedRect.Pitch)));
                imageData[index / 4] = D3DCOLOR_RGBA(r, g, b, a);
            }
        }

        pTexture->UnlockRect(0);
    }

    // TODO: Should allow per texture... or just change renderer API so its global ?
    mDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    mDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);

    // TODO: Copy/and or convert input pixels to texture
    return DxToTextureHandle(pTexture);
}

void DirectX9Renderer::DestroyTextures()
{
    if (!mDestroyTextureList.empty())
    {
        for (size_t i = 0; i < mDestroyTextureList.size(); ++i)
        {
            const LPDIRECT3DTEXTURE9 tex = TextureHandleToDx(mDestroyTextureList[i]);
            tex->Release();
        }
        mDestroyTextureList.clear();
    }
}

const char* DirectX9Renderer::Name() const
{
    return "Direct3D 9";
}
