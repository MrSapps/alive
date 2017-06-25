#pragma once

#include "abstractrenderer.hpp"
#include "SDL.h"

class OpenGLRenderer : public AbstractRenderer
{
public:
    OpenGLRenderer(SDL_Window* window);
    ~OpenGLRenderer();

    virtual TextureHandle CreateTexture(eTextureFormats internalFormat, u32 width, u32 height, eTextureFormats inputFormat, const void *pixels, bool interpolation) override;
    virtual void DestroyTextures() override;
    virtual const char* Name() const override;
    virtual void SetVSync(bool on) override;

private:
    void SetWorldMatrix();
    void SetScreenMatrix();

    virtual void OnSetRenderState(CmdState& info) override;

    virtual void ClearFrameBufferImpl(f32 r, f32 g, f32 b, f32 a) override;
    virtual void RenderCommandsImpl() override;
    void InitGL(SDL_Window* window);

    void ImGuiRender() override;
    void ImGuiRender(struct ImDrawData* data, std::unique_ptr<class Vao>& vao, std::unique_ptr<class BufferObject>& vbo, std::unique_ptr<class BufferObject>& ibo);

    bool CreateShadersAndBufferObjects();

    SDL_GLContext mContext = nullptr;
    SDL_Window* mWindow = nullptr;

    std::unique_ptr<class Vao> mRendererVao;
    std::unique_ptr<class BufferObject> mRendererVbo;
    std::unique_ptr<class BufferObject> mRendererIbo;

    std::unique_ptr<class Vao> mGuiVao;
    std::unique_ptr<class BufferObject> mGuiVbo;
    std::unique_ptr<class BufferObject> mGuiIbo;

    std::unique_ptr<class Shader> mShader;

    int mAttribLocationTex = 0;
    int mAttribLocationProjMtx = 0;
    int mAttribLocationPosition = 0;
    int mAttribLocationUV = 0;
    int mAttribLocationColor = 0;
};