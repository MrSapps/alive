#include "openglrenderer.hpp"

#include "imgui/imgui.h"
#include "oddlib/exceptions.hpp"
#include <GL/gl3w.h>
#ifdef WIN32_LEAN_AND_MEAN
#undef WIN32_LEAN_AND_MEAN
#endif
#include "SDL_opengl.h"

#ifdef NDEBUG
#   define GL(x) x
#else
static inline void assertOnGlError(const char *msg)
{
    GLenum error = glGetError();
    if (error != GL_NO_ERROR)
    {
        LOG_ERROR("GL ERROR: " << error << ", " << msg);
        assert(0 && "GL ERROR");
    }
}
#   define GL(x) do { assertOnGlError("before " #x); x; assertOnGlError("after " #x); } while(0)
#endif

class BufferObject
{
public:
    BufferObject(u32 type)
        : mType(type)
    {
        GL(glGenBuffers(1, &mBo));
    }

    ~BufferObject()
    {
        if (mBo)
        {
            GL(glDeleteBuffers(1, &mBo));
        }
    }

    void Bind()
    {
        GL(glBindBuffer(mType, mBo));
    }

    template<class T>
    void SetData(int size, const T* verts)
    {
        Bind();
        GL(glBufferData(mType, size * sizeof(typename std::remove_pointer<T>::type), verts, GL_STREAM_DRAW));
    }

    u32 mType;
    u32 mBo;
};

class Vao
{
public:
    Vao()
    {
        GL(glGenVertexArrays(1, &mVao));
    }

    ~Vao()
    {
        if (mVao)
        {
            GL(glDeleteVertexArrays(1, &mVao));
        }
    }

    void Bind()
    {
        GL(glBindVertexArray(mVao));
    }

    void BindAttributes(std::unique_ptr<class BufferObject>& vbo, int posAttr, int colourAttr, int uvAttr)
    {
        vbo->Bind();

        GL(glEnableVertexAttribArray(posAttr));
        GL(glEnableVertexAttribArray(uvAttr));
        GL(glEnableVertexAttribArray(colourAttr));

#define OFFSETOF(TYPE, ELEMENT) ((size_t)&(((TYPE *)0)->ELEMENT))
        GL(glVertexAttribPointer(posAttr, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, pos)));
        GL(glVertexAttribPointer(uvAttr, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, uv)));
        GL(glVertexAttribPointer(colourAttr, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, col)));
#undef OFFSETOF

    }

    u32 mVao;
};

class ShaderProgram
{
public:
    ShaderProgram()
    {
        
    }

    ~ShaderProgram()
    {
        if (mShaderProgram)
        {
            GL(glDeleteShader(mShaderProgram));
        }
    }

    void Create(int type)
    {
        mShaderProgram = glCreateShader(type);
    }

    void Compile(const char** shaderSource)
    {
        GL(glShaderSource(mShaderProgram, 1, shaderSource, 0));
        GL(glCompileShader(mShaderProgram));
    }

    int mShaderProgram;
};

class Shader
{
public:
    Shader()
    {
        mShader = glCreateProgram();
        mVertexShader.Create(GL_VERTEX_SHADER);
        mFragmentShader.Create(GL_FRAGMENT_SHADER);
    }

    ~Shader()
    {
        GL(glDeleteProgram(mShader));
    }

    void Link()
    {
        GL(glLinkProgram(mShader));
    }

    void AddShader(ShaderProgram& shader)
    {
        GL(glAttachShader(mShader, shader.mShaderProgram));
    }

    GLint Attribute(const char* name)
    {
        GLint ret = glGetAttribLocation(mShader, name);
        if (ret < 0)
        {
            throw Oddlib::Exception((std::string("Failed to find attribute: ") + name).c_str());
        }
        return ret;
    }

    GLint Uniform(const char* name)
    {
        GLint ret = glGetUniformLocation(mShader, name);
        if (ret < 0)
        {
            throw Oddlib::Exception((std::string("Failed to find uniform: ") + name).c_str());
        }
        return ret;
    }

    void Use()
    {
        GL(glUseProgram(mShader));
    }

    int mShader;
    ShaderProgram mVertexShader;
    ShaderProgram mFragmentShader;
};


// TODO: Error message
#define ALIVE_FATAL_ERROR() abort()

const static GLchar* kVertexShader =
    "#version 330\n"
    "uniform mat4 ProjMtx;\n"
    "in vec2 Position;\n"
    "in vec2 UV;\n"
    "in vec4 Color;\n"
    "out vec2 Frag_UV;\n"
    "out vec4 Frag_Color;\n"
    "void main()\n"
    "{\n"
    "   Frag_UV = UV;\n"
    "   Frag_Color = Color;\n"
    "   gl_Position = ProjMtx * vec4(Position.xy,0,1);\n"
    "}\n";

const static GLchar* kFragmentShader =
    "#version 330\n"
    "uniform sampler2D Texture;\n"
    "in vec2 Frag_UV;\n"
    "in vec4 Frag_Color;\n"
    "out vec4 Out_Color;\n"
    "void main()\n"
    "{\n"
    "   Out_Color = Frag_Color * texture( Texture, Frag_UV.st);\n"
    "}\n";

bool OpenGLRenderer::CreateShadersAndBufferObjects()
{
    mShader = std::make_unique<Shader>();
    mShader->mVertexShader.Compile(&kVertexShader);
    mShader->mFragmentShader.Compile(&kFragmentShader);
    mShader->AddShader(mShader->mVertexShader);
    mShader->AddShader(mShader->mFragmentShader);
    mShader->Link();

    mAttribLocationTex = mShader->Uniform("Texture");
    mAttribLocationProjMtx = mShader->Uniform("ProjMtx");
    mAttribLocationPosition = mShader->Attribute("Position");
    mAttribLocationUV = mShader->Attribute("UV");
    mAttribLocationColor = mShader->Attribute("Color");

    mGuiVbo = std::make_unique<BufferObject>(GL_ARRAY_BUFFER);
    mGuiIbo = std::make_unique<BufferObject>(GL_ELEMENT_ARRAY_BUFFER);
    mGuiVao = std::make_unique<Vao>();
    mGuiVao->Bind();
    mGuiVao->BindAttributes(mGuiVbo, mAttribLocationPosition, mAttribLocationColor, mAttribLocationUV);

    mRendererVbo = std::make_unique<BufferObject>(GL_ARRAY_BUFFER);
    mRendererIbo = std::make_unique<BufferObject>(GL_ELEMENT_ARRAY_BUFFER);
    mRendererVao = std::make_unique<Vao>();
    mRendererVao->Bind();
    mGuiVao->BindAttributes(mRendererVbo, mAttribLocationPosition, mAttribLocationColor, mAttribLocationUV);

    return true;
}

OpenGLRenderer::OpenGLRenderer(SDL_Window* window)
    : mWindow(window)
{
    InitGL(window);
    CreateShadersAndBufferObjects();
}

OpenGLRenderer::~OpenGLRenderer()
{
    SDL_GL_DeleteContext(mContext);
}

void OpenGLRenderer::SetWorldMatrix()
{
    glm::mat4 mat = mProjection * mView;
    mShader->Use();
    glUniform1i(mAttribLocationTex, 0);
    glUniformMatrix4fv(mAttribLocationProjMtx, 1, GL_FALSE, &mat[0][0]);
}

void OpenGLRenderer::SetScreenMatrix()
{
    ImGuiIO& io = ImGui::GetIO();
    const float ortho_projection[4][4] =
    {
        { 2.0f / io.DisplaySize.x, 0.0f,                   0.0f, 0.0f },
        { 0.0f,                  2.0f / -io.DisplaySize.y, 0.0f, 0.0f },
        { 0.0f,                  0.0f,                  -1.0f, 0.0f },
        { -1.0f,                  1.0f,                   0.0f, 1.0f },
    };
    mShader->Use();
    glUniform1i(mAttribLocationTex, 0);
    glUniformMatrix4fv(mAttribLocationProjMtx, 1, GL_FALSE, &ortho_projection[0][0]);
}

void OpenGLRenderer::OnSetRenderState(CmdState& info)
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
        //ToBlendMode(info.mBlendMode);
    }
}

void OpenGLRenderer::ClearFrameBufferImpl(f32 r, f32 g, f32 b, f32 a)
{
    GL(glClearColor(r, g, b, a));
    GL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT));
}

static int ToGLFormat(AbstractRenderer::eTextureFormats format)
{
    switch (format)
    {
    case AbstractRenderer::eTextureFormats::eRGBA:
        return GL_RGBA;
    case AbstractRenderer::eTextureFormats::eRGB:
        return GL_RGB;
    case AbstractRenderer::eTextureFormats::eA:
        return GL_ALPHA;
    }
    ALIVE_FATAL_ERROR();
}


struct BlendMode
{
    u32 srcFactor;
    u32 dstFactor;
    u32 equation;
    f32 colorMul;

    // Some usual blend modes
    static BlendMode normal();
    static BlendMode additive();
    static BlendMode subtractive();
    static BlendMode opaque();
    static BlendMode B100F100();
};

BlendMode BlendMode::normal()
{
    BlendMode b = {};
    b.srcFactor = GL_SRC_ALPHA;
    b.dstFactor = GL_ONE_MINUS_SRC_ALPHA;
    b.equation = GL_FUNC_ADD;
    b.colorMul = 1.0f;
    return b;
}

BlendMode BlendMode::additive()
{
    BlendMode b = {};
    b.srcFactor = GL_SRC_ALPHA;
    b.dstFactor = GL_ONE;
    b.equation = GL_FUNC_ADD;
    b.colorMul = 1.0f;
    return b;
}

BlendMode BlendMode::subtractive()
{
    // Not sure if this is correct formula. Needs testing.
    BlendMode b = {};
    b.srcFactor = GL_SRC_ALPHA;
    b.dstFactor = GL_ONE;
    b.equation = GL_FUNC_REVERSE_SUBTRACT;
    b.colorMul = 1.0f;
    return b;
}

BlendMode BlendMode::opaque()
{
    BlendMode b = {};
    b.srcFactor = GL_ONE;
    b.dstFactor = GL_ZERO;
    b.equation = GL_FUNC_ADD;
    b.colorMul = 1.0f;
    return b;
}

BlendMode BlendMode::B100F100()
{
    BlendMode b = {};
    b.srcFactor = GL_ONE;
    b.dstFactor = GL_ONE;
    b.equation = GL_FUNC_ADD;
    b.colorMul = 1.0f; // This should be less, probably
    return b;
}

/*
static inline BlendMode ToBlendMode(const AbstractRenderer::eBlendModes mode)
{
    switch(mode)
    {
    case AbstractRenderer::eBlendModes::eAdditive:
        return BlendMode::additive();
    case AbstractRenderer::eBlendModes::eB100F100:
        return BlendMode::B100F100();
    case AbstractRenderer::eBlendModes::eNormal:
        return BlendMode::normal();
    case AbstractRenderer::eBlendModes::eOpaque:
        return BlendMode::opaque();
    case AbstractRenderer::eBlendModes::eSubtractive:
        return BlendMode::subtractive();
    }

    return BlendMode::normal();
}
*/

static inline TextureHandle GLToTextureHandle(const GLuint textureNumber)
{
    TextureHandle r;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4312) // 'reinterpret_cast': conversion from 'const GLuint' to 'void *' of greater size
#endif
    r.mData = reinterpret_cast<void*>(textureNumber);
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    return r;
}

static inline GLuint TextureHandleToGL(TextureHandle handle)
{
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4311) // 'reinterpret_cast': pointer truncation from 'void *' to 'GLuint'
#endif
    return static_cast<GLuint>(reinterpret_cast<uintptr_t>(handle.mData));
#ifdef _MSC_VER
#pragma warning(pop)
#endif
}

void OpenGLRenderer::ImGuiRender(ImDrawData* draw_data, std::unique_ptr<Vao>& vao, std::unique_ptr<BufferObject>& vbo, std::unique_ptr<BufferObject>& ibo)
{
    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
    ImGuiIO& io = ImGui::GetIO();
    int fb_width = (int)(io.DisplaySize.x * io.DisplayFramebufferScale.x);
    int fb_height = (int)(io.DisplaySize.y * io.DisplayFramebufferScale.y);
    if (fb_width == 0 || fb_height == 0)
    {
        return;
    }

    // TODO: Check with Hi-DPI
    //draw_data->ScaleClipRects(io.DisplayFramebufferScale);
    
    // Backup GL state
    GLint last_active_texture; glGetIntegerv(GL_ACTIVE_TEXTURE, &last_active_texture);
    glActiveTexture(GL_TEXTURE0);
    GLint last_program; glGetIntegerv(GL_CURRENT_PROGRAM, &last_program);
    GLint last_texture; glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
    GLint last_array_buffer; glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
    GLint last_element_array_buffer; glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &last_element_array_buffer);
    GLint last_vertex_array; glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vertex_array);
    GLint last_blend_src_rgb; glGetIntegerv(GL_BLEND_SRC_RGB, &last_blend_src_rgb);
    GLint last_blend_dst_rgb; glGetIntegerv(GL_BLEND_DST_RGB, &last_blend_dst_rgb);
    GLint last_blend_src_alpha; glGetIntegerv(GL_BLEND_SRC_ALPHA, &last_blend_src_alpha);
    GLint last_blend_dst_alpha; glGetIntegerv(GL_BLEND_DST_ALPHA, &last_blend_dst_alpha);
    GLint last_blend_equation_rgb; glGetIntegerv(GL_BLEND_EQUATION_RGB, &last_blend_equation_rgb);
    GLint last_blend_equation_alpha; glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &last_blend_equation_alpha);
    GLint last_viewport[4]; glGetIntegerv(GL_VIEWPORT, last_viewport);
    GLint last_scissor_box[4]; glGetIntegerv(GL_SCISSOR_BOX, last_scissor_box);
    GLboolean last_enable_blend = glIsEnabled(GL_BLEND);
    GLboolean last_enable_cull_face = glIsEnabled(GL_CULL_FACE);
    GLboolean last_enable_depth_test = glIsEnabled(GL_DEPTH_TEST);
    GLboolean last_enable_scissor_test = glIsEnabled(GL_SCISSOR_TEST);
    

    // Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);

    // Setup viewport, orthographic projection matrix
    glViewport(0, 0, (GLsizei)fb_width, (GLsizei)fb_height);
    SetScreenMatrix();

    vao->Bind();

    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        const ImDrawIdx* idx_buffer_offset = 0;

        vbo->SetData(cmd_list->VtxBuffer.Size, cmd_list->VtxBuffer.Data);
        ibo->SetData(cmd_list->IdxBuffer.size(), cmd_list->IdxBuffer.Data);

        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback)
            {
                pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->TextureId);
                glScissor((int)pcmd->ClipRect.x, (int)(fb_height - pcmd->ClipRect.w), (int)(pcmd->ClipRect.z - pcmd->ClipRect.x), (int)(pcmd->ClipRect.w - pcmd->ClipRect.y));
                glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, idx_buffer_offset);
            }
            idx_buffer_offset += pcmd->ElemCount;
        }
    }

    
    // Restore modified GL state
    glBindVertexArray(last_vertex_array);
    glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, last_element_array_buffer);
    glBlendEquationSeparate(last_blend_equation_rgb, last_blend_equation_alpha);
    glBlendFuncSeparate(last_blend_src_rgb, last_blend_dst_rgb, last_blend_src_alpha, last_blend_dst_alpha);
    if (last_enable_blend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
    if (last_enable_cull_face) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
    if (last_enable_depth_test) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    if (last_enable_scissor_test) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
    glViewport(last_viewport[0], last_viewport[1], (GLsizei)last_viewport[2], (GLsizei)last_viewport[3]);
    glScissor(last_scissor_box[0], last_scissor_box[1], (GLsizei)last_scissor_box[2], (GLsizei)last_scissor_box[3]);
}

void OpenGLRenderer::ImGuiRender()
{
    ImDrawData* data = ImGui::GetDrawData();
    if (data)
    {
        ImGuiRender(data, mGuiVao, mGuiVbo, mGuiIbo);
    }
}

void OpenGLRenderer::RenderCommandsImpl()
{
    GL(glViewport(0, 0, mW, mH));

    if (mRenderDrawLists.empty() == false)
    {
        ImGuiRender(&mRenderDrawData, mRendererVao, mRendererVbo, mRendererIbo);
    }

    DestroyTextures();

    GLenum error = glGetError();
    if (error != GL_NO_ERROR)
    {
        LOG_ERROR("glGetError:" << error);
    }

    SDL_GL_SwapWindow(mWindow);
}

void OpenGLRenderer::SetVSync(bool on)
{
    TRACE_ENTRYEXIT;
    SDL_GL_SetSwapInterval(on ? 1 : 0);
}

TextureHandle OpenGLRenderer::CreateTexture(eTextureFormats internalFormat, u32 width, u32 height, eTextureFormats inputFormat, const void* pixels, bool interpolation)
{
    static std::vector<u32> converted; // Shared scratch buffer - not thread safe, but then GL isn't thread safe anyway
    if (inputFormat == AbstractRenderer::eTextureFormats::eA)
    {
        converted.resize(width*height);
        const u8* alphaPixels = reinterpret_cast<const u8*>(pixels);
        u32 srcIdx = 0;
        u32 dstIdx = 0;
        for (u32 y = 0; y < height; ++y)
        {
            for (u32 x = 0; x < width; x++)
            {
                const u8 a = alphaPixels[srcIdx++];
                converted[dstIdx++] = ColourU8{ 255,255,255, a }.To32Bit();
            }
        }
        inputFormat = AbstractRenderer::eTextureFormats::eRGBA;
    }

    GLuint tex = 0;
    GL(glGenTextures(1, &tex));
    GL(glBindTexture(GL_TEXTURE_2D, tex));
    GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, interpolation ? GL_LINEAR : GL_NEAREST));
    GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, interpolation ? GL_LINEAR : GL_NEAREST));
    GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

    GL(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));
    GL(glTexImage2D(GL_TEXTURE_2D, 0,
        ToGLFormat(internalFormat),
        width, height, 0,
        ToGLFormat(inputFormat),
        GL_UNSIGNED_BYTE,
        converted.empty() ? pixels : converted.data()));
    
    converted.clear();

    return GLToTextureHandle(tex);
}

void OpenGLRenderer::DestroyTextures()
{
    if (!mDestroyTextureList.empty())
    {
        for (size_t i = 0; i < mDestroyTextureList.size(); ++i)
        {
            const GLuint tex = TextureHandleToGL(mDestroyTextureList[i]);
            GL(glDeleteTextures(1, &tex));
        }
        mDestroyTextureList.clear();
    }
}

const char* OpenGLRenderer::Name() const
{
    return "OpenGL 3.0";
}

void OpenGLRenderer::InitGL(SDL_Window* window)
{
    // SDL Defaults
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 3);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 3);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 2);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 0);

    // Overrides
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);

    mContext = SDL_GL_CreateContext(window);
    if (!mContext)
    {
        throw Oddlib::Exception((std::string("Failed to create GL context: ") + SDL_GetError()).c_str());
    }

    int r = 0;
    int g = 0;
    int b = 0;
    int a = 0;
    int bufferSize = 0;
    int doubleBuffer = 0;
    SDL_GL_GetAttribute(SDL_GL_RED_SIZE, &r);
    SDL_GL_GetAttribute(SDL_GL_GREEN_SIZE, &g);
    SDL_GL_GetAttribute(SDL_GL_BLUE_SIZE, &b);
    SDL_GL_GetAttribute(SDL_GL_ALPHA_SIZE, &a);
    SDL_GL_GetAttribute(SDL_GL_BUFFER_SIZE, &bufferSize);
    SDL_GL_GetAttribute(SDL_GL_DOUBLEBUFFER, &doubleBuffer);
    LOG_INFO("GL settings r " << r << " g " << g << " b " << b << " bufferSize " << bufferSize << " double buffer " << doubleBuffer);

    SDL_GL_SetSwapInterval(1);

    if (gl3wInit())
    {
        throw Oddlib::Exception("failed to initialize OpenGL");
    }

    if (!gl3wIsSupported(3, 0))
    {
        throw Oddlib::Exception("OpenGL 3.0 not supported");
    }
}
