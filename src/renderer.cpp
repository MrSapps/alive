#include "renderer.hpp"
#include "oddlib/exceptions.hpp"
#include "logger.hpp"
#include "proxy_nanovg.h"
#define NANOVG_GLES2_IMPLEMENTATION
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4201) // nonstandard extension used : nameless struct/union
#pragma warning(disable:4244) // conversion from 'int' to 'float', possible loss of data
#if _MSC_VER >= 1900
#pragma warning(disable:4459) // declaration of 'defaultFBO' hides global declaration
#endif
#endif
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif
#include "nanovg_gl.h"
#include "nanovg_gl_utils.h"
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#include <algorithm>
#include <cassert>
#include "SDL.h"
#include "SDL_pixels.h"

//static GLuint fontTex;
static GLuint       g_FontTexture = 0;

// TODO: Error message
#define ALIVE_FATAL_ERROR() abort()

#ifdef NDEBUG
#   define GL(x) x
#else
static void assertOnGlError(const char *msg)
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

struct TriMeshVertex
{
    float pos[2];
    float uv[2];
    float color[4];
};

typedef struct VertexAttrib
{
    const GLchar *name;
    GLint size;
    GLenum type;
    bool floating; // False for integer attribs
    GLboolean normalized;
    size_t offset;
} VertexAttrib;

void vertex_attributes(const VertexAttrib **attribs, int *count)
{
    static VertexAttrib tri_attribs[] = {
        { "a_pos", 3, GL_FLOAT, true, false, offsetof(TriMeshVertex, pos) },
        { "a_uv", 3, GL_FLOAT, true, false, offsetof(TriMeshVertex, uv) },
        { "a_color", 4, GL_FLOAT, true, false, offsetof(TriMeshVertex, color) },
    };
    if (attribs)
        *attribs = tri_attribs;
    *count = sizeof(tri_attribs) / sizeof(*tri_attribs);
}

Vao create_vao(int max_v_count, int max_i_count)
{
    int attrib_count;
    const VertexAttrib *attribs;
    vertex_attributes(
        &attribs,
        &attrib_count);

    Vao vao = {};
    vao.v_size = sizeof(TriMeshVertex);
    vao.v_capacity = max_v_count;
    vao.i_capacity = max_i_count;

    GL(glGenVertexArrays(1, &vao.vao_id));
    GL(glGenBuffers(1, &vao.vbo_id));
    GL(glGenBuffers(1, &vao.ibo_id));

    bind_vao(&vao);

    for (int i = 0; i < attrib_count; ++i) {
        GL(glEnableVertexAttribArray(i));
        if (attribs[i].floating) {
            GL(glVertexAttribPointer(
                i,
                attribs[i].size,
                attribs[i].type,
                attribs[i].normalized,
                vao.v_size,
                (const GLvoid*)attribs[i].offset));
        }
        else
        {
            GL(glVertexAttribIPointer(
                i,
                attribs[i].size,
                attribs[i].type,
                vao.v_size,
                (const GLvoid*)attribs[i].offset));
        }
    }

    GL(glBufferData(GL_ARRAY_BUFFER, vao.v_size*max_v_count, NULL, GL_STATIC_DRAW));
    if (vao.ibo_id)
        GL(glBufferData(GL_ELEMENT_ARRAY_BUFFER,
            sizeof(MeshIndexType)*max_i_count, NULL, GL_STATIC_DRAW));
    return vao;
}

void destroy_vao(Vao *vao)
{
    assert(vao->vao_id);

    bind_vao(vao);

    int attrib_count;
    vertex_attributes(NULL, &attrib_count);
    for (int i = 0; i < attrib_count; ++i)
        GL(glDisableVertexAttribArray(i));

    unbind_vao();

    GL(glDeleteVertexArrays(1, &vao->vao_id));
    GL(glDeleteBuffers(1, &vao->vbo_id));
    if (vao->ibo_id)
        GL(glDeleteBuffers(1, &vao->ibo_id));
    vao->vao_id = vao->vbo_id = vao->ibo_id = 0;
}

void bind_vao(const Vao *vao)
{
    assert(vao && vao->vao_id);
    GL(glBindVertexArray(vao->vao_id));
    GL(glBindBuffer(GL_ARRAY_BUFFER, vao->vbo_id));
    if (vao->ibo_id)
        GL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vao->ibo_id));
}

void unbind_vao()
{
    GL(glBindVertexArray(0));
    GL(glBindBuffer(GL_ARRAY_BUFFER, 0));
    GL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
}

void add_vertices_to_vao(Vao *vao, void *vertices, int count)
{
    assert(vao->v_count + count <= vao->v_capacity);
    GL(glBufferSubData(GL_ARRAY_BUFFER,
        vao->v_size*vao->v_count,
        vao->v_size*count,
        vertices));
    vao->v_count += count;
}

void add_indices_to_vao(Vao *vao, MeshIndexType *indices, int count)
{
    assert(vao->ibo_id);
    assert(vao->i_count + count <= vao->i_capacity);
    GL(glBufferSubData(GL_ELEMENT_ARRAY_BUFFER,
        sizeof(MeshIndexType)*vao->i_count,
        sizeof(MeshIndexType)*count,
        indices));
    vao->i_count += count;
}

void reset_vao_mesh(Vao *vao)
{
    vao->v_count = vao->i_count = 0;
}

void draw_vao(const Vao *vao)
{
    if (vao->ibo_id) {
        GL(glDrawRangeElements(GL_TRIANGLES,
            0, vao->i_count, vao->i_count, MESH_INDEX_GL_TYPE, 0));
    }
    else {
        GL(glDrawArrays(GL_POINTS, 0, vao->v_count));
    }
}

GLuint createShader(GLenum type, const char *shaderSrc)
{
    GLuint shader;
    GLint compiled;

    GL(shader = glCreateShader(type));
    if (shader == 0)
        return 0;

    GL(glShaderSource(shader, 1, &shaderSrc, NULL));
    GL(glCompileShader(shader));
    GL(glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled));

    if (!compiled)
    {
        GLint infoLen = 0;
        GL(glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen));
        if (infoLen > 1)
        {
            std::vector<char> infoLog(sizeof(char) * (infoLen+1));
            GL(glGetShaderInfoLog(shader, infoLen, NULL, infoLog.data()));
            LOG_INFO("Error compiling shader: " << infoLog.data());

            ALIVE_FATAL_ERROR();
        }
        return 0;
    }
    return shader;
}
Renderer::Renderer(const char *fontPath)
{
    { // Vector rendering init
        LOG_INFO("Creating nanovg context");

#ifdef _DEBUG
        mNanoVg = nvgCreateGLES2(NVG_ANTIALIAS | NVG_STENCIL_STROKES | NVG_DEBUG);
#else
        mNanoVg = nvgCreateGLES2(NVG_ANTIALIAS | NVG_STENCIL_STROKES);
#endif

        if (!mNanoVg)
        {
            // TODO: Don't throw
            throw Oddlib::Exception("Couldn't create nanovg gl3 context");
        }

        LOG_INFO("Creating nanovg framebuffer");

        // TODO: Should dynamic be set to the window size * 2
        mNanoVgFrameBuffer = nvgluCreateFramebuffer(mNanoVg, 800 * 2, 600 * 2, 0);
        if (!mNanoVgFrameBuffer)
        {
            // TODO: Don't throw
            throw Oddlib::Exception("Couldn't create nanovg framebuffer");
        }

        { // Load nanovg font
            int font = nvgCreateFont(mNanoVg, "sans", fontPath);
            if (font == -1)
            {
                LOG_ERROR("Could not add font regular");
                // TODO: Don't throw
                throw Oddlib::Exception("Failed to load fonts");
            }
        }

        LOG_INFO("Nanovg initialized");
    }

    { // Textured quad rendering init
        const char vsString[] =
            "#version 120\n"
            "attribute vec2 a_pos;        \n"
            "attribute vec2 a_uv;         \n"
            "attribute vec4 a_color;      \n"
            "varying vec2 v_uv;               \n"
            "varying vec4 v_color;            \n"
            "void main()                  \n"
            "{                            \n"
            "    v_uv = a_uv;             \n"
            "    v_color = a_color;       \n"
            "    gl_Position = vec4(a_pos, 0, 1); \n"
            "}                            \n";
        const char fsString[] =
            "#version 120\n"
            "uniform sampler2D u_tex;\n"
            "varying vec2 v_uv; \n"
            "varying vec4 v_color; \n"
            "void main()                                  \n"
            "{                                            \n"
            "  gl_FragColor = v_color*texture2D(u_tex, v_uv);\n"
            "}                                            \n";

        mVs = createShader(GL_VERTEX_SHADER, vsString);
        mFs = createShader(GL_FRAGMENT_SHADER, fsString);
        GL(mProgram = glCreateProgram());

        if (mProgram == 0)
            ALIVE_FATAL_ERROR();

        GL(glAttachShader(mProgram, mVs));
        GL(glAttachShader(mProgram, mFs));

        GL(glBindAttribLocation(mProgram, 0, "v_pos"));
        GL(glLinkProgram(mProgram));

        GLint linked;
        GL(glGetProgramiv(mProgram, GL_LINK_STATUS, &linked));
        if (!linked)
        {
            GLint infoLen = 0;

            GL(glGetProgramiv(mProgram, GL_INFO_LOG_LENGTH, &infoLen));

            if (infoLen > 1)
            {
                std::vector<char> infoLog(sizeof(char) * (infoLen+1));
                GL(glGetProgramInfoLog(mProgram, infoLen, NULL, infoLog.data()));
                LOG_INFO("Error linking program " << infoLog.data());
            }
            ALIVE_FATAL_ERROR();
        }

        mQuadVao = create_vao(4, 6);
        unbind_vao();
    }

    // These should be large enough so that no allocations are done during game
    mDrawCmds.reserve(1024 * 4);
    mLayerStack.reserve(64);
    mDestroyTextureList.reserve(8);
}

Renderer::~Renderer()
{
    { // Delete vector rendering
        if (g_FontTexture)
        {
            GL(glDeleteTextures(1, &g_FontTexture));
            g_FontTexture = 0;
        }

        if (mNanoVgFrameBuffer)
        {
            nvgluDeleteFramebuffer(mNanoVgFrameBuffer);
        }

        if (mNanoVg)
        {
            nvgDeleteGLES2(mNanoVg);
        }
    }

    { // Delete textured quad rendering
        destroy_vao(&mQuadVao);

        GL(glDeleteShader(mVs));
        GL(glDeleteShader(mFs));
        GL(glDeleteProgram(mProgram));
    }
}

void Renderer::beginFrame(int w, int h)
{
    GL(glClearColor(0.4f, 0.4f, 0.4f, 1.0f));
    GL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT));

    mW = w;
    mH = h;

    assert(mDrawCmds.empty());
}

void Renderer::endFrame()
{
    assert(mLayerStack.empty());

    // This is the primary reason for buffering drawing command. Call order doesn't determine draw order, but layers do.
    std::stable_sort(mDrawCmds.begin(), mDrawCmds.end(), [](const DrawCmd& a, const DrawCmd& b)
    {
        return a.layer < b.layer;
    });

    // Actual rendering
    GL(glViewport(0, 0, mW, mH));
    nvgResetTransform(mNanoVg);
    bool vectorModeOn = false;
    for (size_t i = 0; i < mDrawCmds.size(); ++i)
    {
        DrawCmd cmd = mDrawCmds[i];
        if (!vectorModeOn && cmd.type != DrawCmdType_quad)
            nvgBeginFrame(mNanoVg, mW, mH, 1.0f); // Start nanovg rendering batch
        else if (vectorModeOn && cmd.type == DrawCmdType_quad)
            nvgEndFrame(mNanoVg); // Stop nanovg rendering batch

        if (vectorModeOn && cmd.type == DrawCmdType_quad)
        { // Start game rendering batch
            GL(glDisable(GL_DEPTH_TEST));
            GL(glDisable(GL_STENCIL_TEST));
            GL(glDisable(GL_CULL_FACE));
            GL(glDisable(GL_SCISSOR_TEST));
            GL(glEnable(GL_BLEND));
            GL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
            GL(glEnable(GL_TEXTURE_2D));
            GL(glDisable(GL_ALPHA_TEST));
        }

        vectorModeOn = (cmd.type != DrawCmdType_quad);

        switch (cmd.type)
        {
        case DrawCmdType_quad:
        {
            int texHandle = cmd.s.integer;
            float x = cmd.s.f[0];
            float y = cmd.s.f[1];
            float w = cmd.s.f[2];
            float h = cmd.s.f[3];

            float white[4] = { 1, 1, 1, 1 };

            TriMeshVertex vert[4] = {};
            vert[0].pos[0] = 2 * x / mW - 1;
            vert[0].pos[1] = -2 * y / mH + 1;
            vert[0].uv[0] = 0;
            vert[0].uv[1] = 0;
            memcpy(vert[0].color, white, sizeof(vert[0].color));

            vert[1].pos[0] = 2 * (x + w) / mW - 1;
            vert[1].pos[1] = -2 * y / mH + 1;
            vert[1].uv[0] = 1;
            vert[1].uv[1] = 0;
            memcpy(vert[1].color, white, sizeof(vert[1].color));

            vert[2].pos[0] = 2 * (x + w) / mW - 1;
            vert[2].pos[1] = -2 * (y + h) / mH + 1;
            vert[2].uv[0] = 1;
            vert[2].uv[1] = 1;
            memcpy(vert[2].color, white, sizeof(vert[2].color));

            vert[3].pos[0] = 2 * x / mW - 1;
            vert[3].pos[1] = -2 * (y + h) / mH + 1;
            vert[3].uv[0] = 0;
            vert[3].uv[1] = 1;
            memcpy(vert[3].color, white, sizeof(vert[3].color));

            static MeshIndexType ind[6] = { 0, 1, 2, 0, 2, 3 };

            // TODO: Batch rendering if becomes bottleneck
            bind_vao(&mQuadVao);
            reset_vao_mesh(&mQuadVao);
            add_vertices_to_vao(&mQuadVao, vert, 4);
            add_indices_to_vao(&mQuadVao, ind, 6);

            GL(glActiveTexture(GL_TEXTURE0));
            GL(glBindTexture(GL_TEXTURE_2D, texHandle));
            GL(glUseProgram(mProgram));
            draw_vao(&mQuadVao);

            unbind_vao();
        } break;
        case DrawCmdType_fillColor: nvgFillColor(mNanoVg, nvgRGBAf(cmd.s.f[0], cmd.s.f[1], cmd.s.f[2], cmd.s.f[3])); break;
        case DrawCmdType_strokeColor: nvgStrokeColor(mNanoVg, nvgRGBAf(cmd.s.f[0], cmd.s.f[1], cmd.s.f[2], cmd.s.f[3])); break;
        case DrawCmdType_strokeWidth: nvgStrokeWidth(mNanoVg, cmd.s.f[0]); break;
        case DrawCmdType_fontSize: nvgFontSize(mNanoVg, cmd.s.f[0]); break;
        case DrawCmdType_fontBlur: nvgFontBlur(mNanoVg, cmd.s.f[0]); break;
        case DrawCmdType_textAlign: nvgTextAlign(mNanoVg, cmd.s.integer); break;
        case DrawCmdType_text: nvgText(mNanoVg, cmd.s.f[0], cmd.s.f[1], cmd.s.str, nullptr); break;
        case DrawCmdType_resetTransform: nvgResetTransform(mNanoVg); break;
        case DrawCmdType_beginPath: nvgBeginPath(mNanoVg); break;
        case DrawCmdType_moveTo: nvgMoveTo(mNanoVg, cmd.s.f[0], cmd.s.f[1]); break;
        case DrawCmdType_lineTo: nvgLineTo(mNanoVg, cmd.s.f[0], cmd.s.f[1]); break;
        case DrawCmdType_closePath: nvgClosePath(mNanoVg); break;
        case DrawCmdType_fill: nvgFill(mNanoVg); break;
        case DrawCmdType_stroke: nvgStroke(mNanoVg); break;
        case DrawCmdType_roundedRect: nvgRoundedRect(mNanoVg, cmd.s.f[0], cmd.s.f[1], cmd.s.f[2], cmd.s.f[3], cmd.s.f[4]); break;
        case DrawCmdType_rect: nvgRect(mNanoVg, cmd.s.f[0], cmd.s.f[1], cmd.s.f[2], cmd.s.f[3]); break;
        case DrawCmdType_circle: nvgCircle(mNanoVg, cmd.s.f[0], cmd.s.f[1], cmd.s.f[2]); break;
        case DrawCmdType_solidPathWinding: nvgPathWinding(mNanoVg, cmd.s.integer ? NVG_SOLID : NVG_HOLE); break;
        case DrawCmdType_fillPaint:
        {
            RenderPaint p = cmd.paint;
            NVGpaint nvp = {};

            static_assert(sizeof(p.xform) == sizeof(nvp.xform), "wrong size");
            static_assert(sizeof(p.extent) == sizeof(nvp.extent), "wrong size");

            memcpy(nvp.xform, p.xform, sizeof(p.xform));
            memcpy(nvp.extent, p.extent, sizeof(p.extent));
            nvp.radius = p.radius;
            nvp.feather = p.feather;
            nvp.innerColor.r = p.innerColor.r;
            nvp.innerColor.g = p.innerColor.g;
            nvp.innerColor.b = p.innerColor.b;
            nvp.innerColor.a = p.innerColor.a;
            nvp.outerColor.r = p.outerColor.r;
            nvp.outerColor.g = p.outerColor.g;
            nvp.outerColor.b = p.outerColor.b;
            nvp.outerColor.a = p.outerColor.a;
            nvp.image = p.image;
            nvgFillPaint(mNanoVg, nvp);
        } break;
        case DrawCmdType_scissor: nvgScissor(mNanoVg, cmd.s.f[0], cmd.s.f[1], cmd.s.f[2], cmd.s.f[3]); break;
        case DrawCmdType_resetScissor: nvgResetScissor(mNanoVg); break;
        default: assert(0 && "Unknown DrawCmdType");
        }
    }
    if (vectorModeOn)
        nvgEndFrame(mNanoVg);
    mDrawCmds.clear(); // Don't release memory, just reset count

    for (size_t i = 0; i < mDestroyTextureList.size(); ++i)
    {
        GLuint tex = (GLuint)mDestroyTextureList[i];
        GL(glDeleteTextures(1, &tex));
    }
    mDestroyTextureList.clear();

    GLenum error = glGetError();
    if (error != GL_NO_ERROR)
    {
        // TODO: Make our own gluErrorString alike function as its marked deprecated on OSX
        // it only needs to stringify about 6 error codes
       // LOG_ERROR(gluErrorString(error));
        LOG_ERROR("glGetError:" << error);
    }
}

void Renderer::beginLayer(int depth)
{
    mLayerStack.push_back(depth);
}

void Renderer::endLayer()
{
    mLayerStack.pop_back();
}

int Renderer::createTexture(GLenum internalFormat, int width, int height, GLenum inputFormat, GLenum colorDataType, const void *pixels, bool interpolation)
{
    GLuint tex;
    GL(glGenTextures(1, &tex));
    GL(glBindTexture(GL_TEXTURE_2D, tex));
    GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, interpolation ? GL_LINEAR : GL_NEAREST));
    GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, interpolation ? GL_LINEAR : GL_NEAREST));
    GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP));
    GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP));

    GL(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));
    GL(glTexImage2D(   GL_TEXTURE_2D, 0,
                    internalFormat,
                    width, height, 0,
                    inputFormat,
                    colorDataType,
                    pixels));

    return tex;
}

void Renderer::destroyTexture(int handle)
{
    if (handle)
    {
        mDestroyTextureList.push_back(handle); // Delay the deletion after drawing current frame
    }
}

void Renderer::drawQuad(int texHandle, float x, float y, float w, float h)
{
    DrawCmd cmd;
    cmd.type = DrawCmdType_quad;
    cmd.s.integer = texHandle;
    cmd.s.f[0] = x;
    cmd.s.f[1] = y;
    cmd.s.f[2] = w;
    cmd.s.f[3] = h;
    pushCmd(cmd);
}

void Renderer::fillColor(Color c)
{
    DrawCmd cmd;
    cmd.type = DrawCmdType_fillColor;
    memcpy(cmd.s.f, &c, sizeof(c));
    pushCmd(cmd);
}

void Renderer::strokeColor(Color c)
{
    DrawCmd cmd;
    cmd.type = DrawCmdType_strokeColor;
    memcpy(cmd.s.f, &c, sizeof(c));
    pushCmd(cmd);
}

void Renderer::strokeWidth(float size)
{
    DrawCmd cmd;
    cmd.type = DrawCmdType_strokeWidth;
    cmd.s.f[0] = size;
    pushCmd(cmd);
}

void Renderer::fontSize(float s)
{
    nvgFontSize(mNanoVg, s); // Must call nanovg because affects text size query

    DrawCmd cmd;
    cmd.type = DrawCmdType_fontSize;
    cmd.s.f[0] = s;
    pushCmd(cmd);
}

void Renderer::fontBlur(float s)
{
    nvgFontBlur(mNanoVg, s); // Must call nanovg because affects text size query

    DrawCmd cmd;
    cmd.type = DrawCmdType_fontBlur;
    cmd.s.f[0] = s;
    pushCmd(cmd);
}

void Renderer::textAlign(int align)
{
    nvgTextAlign(mNanoVg, align); // Must call nanovg because affects text size query

    DrawCmd cmd;
    cmd.type = DrawCmdType_textAlign;
    cmd.s.integer = align;
    pushCmd(cmd);
}

void Renderer::text(float x, float y, const char *msg)
{
    DrawCmd cmd;
    cmd.type = DrawCmdType_text;
    cmd.s.f[0] = x;
    cmd.s.f[1] = y;
    strncpy(cmd.s.str, msg, sizeof(cmd.s.str));
    cmd.s.str[sizeof(cmd.s.str) - 1] = '\0';
    pushCmd(cmd);
}

void Renderer::resetTransform()
{
    DrawCmd cmd;
    cmd.type = DrawCmdType_resetTransform;
    pushCmd(cmd);
}

void Renderer::beginPath()
{
    DrawCmd cmd;
    cmd.type = DrawCmdType_beginPath;
    pushCmd(cmd);
}

void Renderer::moveTo(float x, float y)
{
    DrawCmd cmd;
    cmd.type = DrawCmdType_moveTo;
    cmd.s.f[0] = x;
    cmd.s.f[1] = y;
    pushCmd(cmd);
}

void Renderer::lineTo(float x, float y)
{
    DrawCmd cmd;
    cmd.type = DrawCmdType_lineTo;
    cmd.s.f[0] = x;
    cmd.s.f[1] = y;
    pushCmd(cmd);
}

void Renderer::closePath()
{
    DrawCmd cmd;
    cmd.type = DrawCmdType_closePath;
    pushCmd(cmd);
}

void Renderer::fill()
{
    DrawCmd cmd;
    cmd.type = DrawCmdType_fill;
    pushCmd(cmd);
}

void Renderer::stroke()
{
    DrawCmd cmd;
    cmd.type = DrawCmdType_stroke;
    pushCmd(cmd);
}

void Renderer::roundedRect(float x, float y, float w, float h, float r)
{
    DrawCmd cmd;
    cmd.type = DrawCmdType_roundedRect;
    cmd.s.f[0] = x;
    cmd.s.f[1] = y;
    cmd.s.f[2] = w;
    cmd.s.f[3] = h;
    cmd.s.f[4] = r;
    pushCmd(cmd);
}

void Renderer::rect(float x, float y, float w, float h)
{
    DrawCmd cmd;
    cmd.type = DrawCmdType_rect;
    cmd.s.f[0] = x;
    cmd.s.f[1] = y;
    cmd.s.f[2] = w;
    cmd.s.f[3] = h;
    pushCmd(cmd);
}

void Renderer::circle(float x, float y, float r)
{
    DrawCmd cmd;
    cmd.type = DrawCmdType_circle;
    cmd.s.f[0] = x;
    cmd.s.f[1] = y;
    cmd.s.f[2] = r;
    pushCmd(cmd);
}

void Renderer::solidPathWinding(bool b)
{
    DrawCmd cmd;
    cmd.type = DrawCmdType_solidPathWinding;
    cmd.s.integer = b;
    pushCmd(cmd);
}

void Renderer::fillPaint(RenderPaint p)
{
    DrawCmd cmd;
    cmd.type = DrawCmdType_fillPaint;
    cmd.paint = p;
    pushCmd(cmd);
}

void Renderer::scissor(float x, float y, float w, float h)
{
    DrawCmd cmd;
    cmd.type = DrawCmdType_scissor;
    cmd.s.f[0] = x;
    cmd.s.f[1] = y;
    cmd.s.f[2] = w;
    cmd.s.f[3] = h;
    pushCmd(cmd);
}

void Renderer::resetScissor()
{
    DrawCmd cmd;
    cmd.type = DrawCmdType_resetScissor;
    pushCmd(cmd);
}

// Not rendering commands

static RenderPaint NVGpaintToRenderPaint(NVGpaint nvp)
{
    RenderPaint p = {};

    static_assert(sizeof(p.xform) == sizeof(nvp.xform), "wrong size");
    static_assert(sizeof(p.extent) == sizeof(nvp.extent), "wrong size");

	memcpy(p.xform, nvp.xform, sizeof(p.xform));
	memcpy(p.extent, nvp.extent, sizeof(p.extent));
    p.radius = nvp.radius;
    p.feather = nvp.feather;
    p.innerColor.r = nvp.innerColor.r;
    p.innerColor.g = nvp.innerColor.g;
    p.innerColor.b = nvp.innerColor.b;
    p.innerColor.a = nvp.innerColor.a;
    p.outerColor.r = nvp.outerColor.r;
    p.outerColor.g = nvp.outerColor.g;
    p.outerColor.b = nvp.outerColor.b;
    p.outerColor.a = nvp.outerColor.a;
    p.image = nvp.image;
    return p;
}

RenderPaint Renderer::linearGradient(float sx, float sy, float ex, float ey, Color sc, Color ec)
{
    NVGpaint nvp = nvgLinearGradient(mNanoVg, sx, sy, ex, ey, nvgRGBAf(sc.r, sc.g, sc.b, sc.a), nvgRGBAf(ec.r, ec.g, ec.b, ec.a));
    return NVGpaintToRenderPaint(nvp);
}

RenderPaint Renderer::boxGradient(float x, float y, float w, float h,
                                  float r, float f, Color icol, Color ocol)
{
    NVGpaint nvp = nvgBoxGradient(mNanoVg, x, y, w, h, r, f, nvgRGBAf(icol.r, icol.g, icol.b, icol.a), nvgRGBAf(ocol.r, ocol.g, ocol.b, ocol.a));
    return NVGpaintToRenderPaint(nvp);
}

RenderPaint Renderer::radialGradient(float cx, float cy, float inr, float outr, Color icol, Color ocol)
{
    NVGpaint nvp = nvgRadialGradient(mNanoVg, cx, cy, inr, outr, 
                      nvgRGBAf(icol.r, icol.g, icol.b, icol.a), nvgRGBAf(ocol.r, ocol.g, ocol.b, ocol.a));
    return NVGpaintToRenderPaint(nvp);
}

void Renderer::textBounds(int x, int y, const char *msg, float bounds[4])
{
    nvgTextBounds(mNanoVg, 1.f*x, 1.f*y, msg, nullptr, bounds);
}

void Renderer::pushCmd(DrawCmd cmd)
{
    cmd.layer = mLayerStack.empty() ? 0 : mLayerStack.back();
    mDrawCmds.push_back(cmd);
}
