#pragma once

#include "abstractrenderer.hpp"
#include "SDL.h"
#include <d3d9.h>

#if !defined(__MINGW32__)
#   pragma warning(push)
#   pragma warning(disable:4265)
#endif
#include <wrl/client.h>
#if !defined(__MINGW32__)
#   pragma warning(pop)
#endif
#include "imgui/imgui.h"

class DirectX9Renderer : public AbstractRenderer
{
public:
    DirectX9Renderer(SDL_Window* window);
    virtual ~DirectX9Renderer() override;
    virtual void SetVSync(bool on) override;

private:
    virtual void OnSetRenderState(CmdState& info) override;

    void SetRendererStates();

    void SetScreenMatrix();
    void SetWorldMatrix();

    virtual void ClearFrameBufferImpl(f32 r, f32 g, f32 b, f32 a) override;
    virtual void RenderCommandsImpl() override;
    virtual TextureHandle CreateTexture(eTextureFormats internalFormat, u32 width, u32 height, eTextureFormats inputFormat, const void *pixels, bool interpolation) override;
    virtual void DestroyTextures() override;
    virtual const char* Name() const override;
    void doDraw(struct ImDrawList* list, int& vtx_offset, int& idx_offset);

    virtual void ImGuiRender() override;
    void ImGuiRender(struct ImDrawData* data, Microsoft::WRL::ComPtr<IDirect3DVertexBuffer9>& vbo, int& vboSize, Microsoft::WRL::ComPtr<IDirect3DIndexBuffer9>& ibo, int& iboSize);
    void GetHWND(SDL_Window* window);
    void PopulatePresentParams(bool vSync);

    HWND mHwnd = nullptr;
    Microsoft::WRL::ComPtr<IDirect3D9> mD3D9;
    Microsoft::WRL::ComPtr<IDirect3DDevice9> mDevice;

    Microsoft::WRL::ComPtr<IDirect3DVertexBuffer9> mGuiVBO;
    Microsoft::WRL::ComPtr<IDirect3DIndexBuffer9> mGuiIBO;

    Microsoft::WRL::ComPtr<IDirect3DVertexBuffer9> mRendererVBO;
    Microsoft::WRL::ComPtr<IDirect3DIndexBuffer9> mRendererIBO;

    D3DPRESENT_PARAMETERS mPresentParameters; // stores the important attributes and 
    D3DMATRIX mProjectionMatrix;              // properties your Direct3D_device will have
    VOID* mData;                              // pointer to beginning of vertex buffer
                                              // actual data to be fed to the vertex buffer
    struct D3DVERTEX
    {
        FLOAT x, y, z;  // from the D3DFVF_XYZ flag
        DWORD colour;   // from the D3DFVF_DIFFUSE flag
        float u, v;     // from the D3DFVF_TEX1 flag 

        const static u32 kFVF = D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1;
    };
    
    int mGuiVertexBufferSize = 5000;
    int mGuiIndexBufferSize = 10000;

    int mRendererVertexBufferSize = 5000;
    int mRendererIndexBufferSize = 10000;

    bool mVsyncEnabled = true;
};
