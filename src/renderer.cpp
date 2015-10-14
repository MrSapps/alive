#include "renderer.hpp"
#include "oddlib/exceptions.hpp"
#include "logger.hpp"
#include "nanovg.h"
#define NANOVG_GLES2_IMPLEMENTATION
#include "nanovg_gl.h"
#include "nanovg_gl_utils.h"

#include <algorithm>
#include <cassert>
#include "SDL.h"
#include "SDL_pixels.h"

static GLuint fontTex;
static GLuint       g_FontTexture = 0;

// TODO: Error message
#define ALIVE_FATAL_ERROR() abort()

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
    int offset;
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

    glGenVertexArrays(1, &vao.vao_id);
    glGenBuffers(1, &vao.vbo_id);
    glGenBuffers(1, &vao.ibo_id);

    bind_vao(&vao);

    for (int i = 0; i < attrib_count; ++i) {
        glEnableVertexAttribArray(i);
        if (attribs[i].floating) {
            glVertexAttribPointer(
                i,
                attribs[i].size,
                attribs[i].type,
                attribs[i].normalized,
                vao.v_size,
                (const GLvoid*)attribs[i].offset);
        }
        else
        {
            glVertexAttribIPointer(
                i,
                attribs[i].size,
                attribs[i].type,
                vao.v_size,
                (const GLvoid*)attribs[i].offset);
        }
    }

    glBufferData(GL_ARRAY_BUFFER, vao.v_size*max_v_count, NULL, GL_STATIC_DRAW);
    if (vao.ibo_id)
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
            sizeof(MeshIndexType)*max_i_count, NULL, GL_STATIC_DRAW);
    return vao;
}

void destroy_vao(Vao *vao)
{
    assert(vao->vao_id);

    bind_vao(vao);

    int attrib_count;
    vertex_attributes(NULL, &attrib_count);
    for (int i = 0; i < attrib_count; ++i)
        glDisableVertexAttribArray(i);

    unbind_vao();

    glDeleteVertexArrays(1, &vao->vao_id);
    glDeleteBuffers(1, &vao->vbo_id);
    if (vao->ibo_id)
        glDeleteBuffers(1, &vao->ibo_id);
    vao->vao_id = vao->vbo_id = vao->ibo_id = 0;
}

void bind_vao(const Vao *vao)
{
    assert(vao && vao->vao_id);
    glBindVertexArray(vao->vao_id);
    glBindBuffer(GL_ARRAY_BUFFER, vao->vbo_id);
    if (vao->ibo_id)
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vao->ibo_id);
}

void unbind_vao()
{
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void add_vertices_to_vao(Vao *vao, void *vertices, int count)
{
    assert(vao->v_count + count <= vao->v_capacity);
    glBufferSubData(GL_ARRAY_BUFFER,
        vao->v_size*vao->v_count,
        vao->v_size*count,
        vertices);
    vao->v_count += count;
}

void add_indices_to_vao(Vao *vao, MeshIndexType *indices, int count)
{
    assert(vao->ibo_id);
    assert(vao->i_count + count <= vao->i_capacity);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER,
        sizeof(MeshIndexType)*vao->i_count,
        sizeof(MeshIndexType)*count,
        indices);
    vao->i_count += count;
}

void reset_vao_mesh(Vao *vao)
{
    vao->v_count = vao->i_count = 0;
}

void draw_vao(const Vao *vao)
{
    if (vao->ibo_id) {
        glDrawRangeElements(GL_TRIANGLES,
            0, vao->i_count, vao->i_count, MESH_INDEX_GL_TYPE, 0);
    }
    else {
        glDrawArrays(GL_POINTS, 0, vao->v_count);
    }
}

GLuint createShader(GLenum type, const char *shaderSrc)
{
    GLuint shader;
    GLint compiled;

    shader = glCreateShader(type);
    if (shader == 0)
        return 0;

    glShaderSource(shader, 1, &shaderSrc, NULL);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

    if (!compiled)
    {
        GLint infoLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen > 1)
        {
            std::vector<char> infoLog(sizeof(char) * (infoLen+1));
            glGetShaderInfoLog(shader, infoLen, NULL, infoLog.data());
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
            "attribute vec2 a_pos;        \n"
            "attribute vec2 a_uv;         \n"
            "attribute vec4 a_color;      \n"
            "out vec2 v_uv;               \n"
            "out vec4 v_color;            \n"
            "void main()                  \n"
            "{                            \n"
            "    v_uv = a_uv;             \n"
            "    v_color = a_color;       \n"
            "    gl_Position = vec4(a_pos, 0, 1); \n"
            "}                            \n";
        const char fsString[] =
            "uniform sampler2D u_tex;\n"
            "in vec2 v_uv; \n"
            "in vec4 v_color; \n"
            "precision mediump float;\n"
            "void main()                                  \n"
            "{                                            \n"
            "  gl_FragColor = v_color*texture2D(u_tex, v_uv);\n"
            "}                                            \n";

        mVs = createShader(GL_VERTEX_SHADER, vsString);
        mFs = createShader(GL_FRAGMENT_SHADER, fsString);
        mProgram = glCreateProgram();

        if (mProgram == 0)
            ALIVE_FATAL_ERROR();

        glAttachShader(mProgram, mVs);
        glAttachShader(mProgram, mFs);

        glBindAttribLocation(mProgram, 0, "v_pos");
        glLinkProgram(mProgram);

        GLint linked;
        glGetProgramiv(mProgram, GL_LINK_STATUS, &linked);
        if (!linked)
        {
            GLint infoLen = 0;

            glGetProgramiv(mProgram, GL_INFO_LOG_LENGTH, &infoLen);

            if (infoLen > 1)
            {
                std::vector<char> infoLog(sizeof(char) * (infoLen+1));
                glGetProgramInfoLog(mProgram, infoLen, NULL, infoLog.data());
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
}

Renderer::~Renderer()
{
    { // Delete vector rendering
        if (g_FontTexture)
        {
            glDeleteTextures(1, &g_FontTexture);
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

        glDeleteShader(mVs);
        glDeleteShader(mFs);
        glDeleteProgram(mProgram);
    }
}

void Renderer::beginFrame(int w, int h)
{
    glClearColor(0.4f, 0.4f, 0.4f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    mW = w;
    mH = h;

    assert(mDrawCmds.empty());
}

struct DrawCmdSort {
    bool operator()(const DrawCmd& a, const DrawCmd& b) const
    {
        return a.layer < b.layer; 
    }
};

void Renderer::endFrame()
{
    assert(mLayerStack.empty());

    // This is the primary reason for buffering drawing command. Call order doesn't determine draw order, but layers do.
    std::stable_sort(mDrawCmds.begin(), mDrawCmds.end(), DrawCmdSort());

    // Actual rendering
    glViewport(0, 0, mW, mH);
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
            glDisable(GL_DEPTH_TEST);
            glDisable(GL_STENCIL_TEST);
            glDisable(GL_CULL_FACE);
            glDisable(GL_SCISSOR_TEST);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glEnable(GL_TEXTURE_2D);
            glDisable(GL_ALPHA_TEST);
        }

        vectorModeOn = (cmd.type != DrawCmdType_quad);

        switch (cmd.type)
        {
        case DrawCmdType_quad:
        {
            int texHandle = cmd.integer;
            float x = cmd.f[0];
            float y = cmd.f[1];
            float w = cmd.f[2];
            float h = cmd.f[3];

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

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texHandle);
            glUseProgram(mProgram);
            draw_vao(&mQuadVao);

            unbind_vao();
        } break;
        case DrawCmdType_fillColor: nvgFillColor(mNanoVg, nvgRGBAf(cmd.f[0], cmd.f[1], cmd.f[2], cmd.f[3])); break;
        case DrawCmdType_strokeColor: nvgStrokeColor(mNanoVg, nvgRGBAf(cmd.f[0], cmd.f[1], cmd.f[2], cmd.f[3])); break;
        case DrawCmdType_strokeWidth: nvgStrokeWidth(mNanoVg, cmd.f[0]); break;
        case DrawCmdType_fontSize: nvgFontSize(mNanoVg, cmd.f[0]); break;
        case DrawCmdType_fontBlur: nvgFontBlur(mNanoVg, cmd.f[0]); break;
        case DrawCmdType_textAlign: nvgTextAlign(mNanoVg, cmd.integer); break;
        case DrawCmdType_text: nvgText(mNanoVg, cmd.f[0], cmd.f[1], cmd.str, nullptr); break;
        case DrawCmdType_resetTransform: nvgResetTransform(mNanoVg); break;
        case DrawCmdType_beginPath: nvgBeginPath(mNanoVg); break;
        case DrawCmdType_moveTo: nvgMoveTo(mNanoVg, cmd.f[0], cmd.f[1]); break;
        case DrawCmdType_lineTo: nvgLineTo(mNanoVg, cmd.f[0], cmd.f[1]); break;
        case DrawCmdType_closePath: nvgClosePath(mNanoVg); break;
        case DrawCmdType_fill: nvgFill(mNanoVg); break;
        case DrawCmdType_stroke: nvgStroke(mNanoVg); break;
        case DrawCmdType_roundedRect: nvgRoundedRect(mNanoVg, cmd.f[0], cmd.f[1], cmd.f[2], cmd.f[3], cmd.f[4]); break;
        case DrawCmdType_rect: nvgRect(mNanoVg, cmd.f[0], cmd.f[1], cmd.f[2], cmd.f[3]); break;
        case DrawCmdType_solidPathWinding: nvgPathWinding(mNanoVg, cmd.integer ? NVG_SOLID : NVG_HOLE); break;
        case DrawCmdType_fillPaint:
        {
            RenderPaint p = cmd.paint;
            NVGpaint nvp = { 0 };

            assert(sizeof(p.xform) == sizeof(nvp.xform));
            assert(sizeof(p.extent) == sizeof(nvp.extent));

            memcpy(nvp.xform, p.xform, sizeof(p.xform));
            memcpy(nvp.extent, p.extent, sizeof(p.xform));
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
        default: assert(0 && "Unknown DrawCmdType");
        }
    }
    if (vectorModeOn)
        nvgEndFrame(mNanoVg);
    mDrawCmds.clear(); // Don't release memory, just reset count

    GLenum error = glGetError();
    if (error != GL_NO_ERROR)
    {
        LOG_ERROR(gluErrorString(error));
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

int Renderer::createTexture(void *pixels, int width, int height, PixelFormat format)
{
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    assert(format == PixelFormat_RGB24 && "TODO: Support other pixel formats");

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(   GL_TEXTURE_2D, 0,
                    GL_RGB, // Internal format
                    width, height, 0,
                    GL_RGB, // Passed format
                    GL_UNSIGNED_BYTE, // Color component datatype
                    pixels);

    return tex;
}

void Renderer::destroyTexture(int handle)
{
    if (handle)
    {
        GLuint tex = (GLuint)handle;
        glDeleteTextures(1, &tex);
    }
}

void Renderer::drawQuad(int texHandle, float x, float y, float w, float h)
{
    DrawCmd cmd;
    cmd.type = DrawCmdType_quad;
    cmd.integer = texHandle;
    cmd.f[0] = x;
    cmd.f[1] = y;
    cmd.f[2] = w;
    cmd.f[3] = h;
    pushCmd(cmd);
}

void Renderer::fillColor(Color c)
{
    DrawCmd cmd;
    cmd.type = DrawCmdType_fillColor;
    memcpy(cmd.f, &c, sizeof(c));
    pushCmd(cmd);
}

void Renderer::strokeColor(Color c)
{
    DrawCmd cmd;
    cmd.type = DrawCmdType_strokeColor;
    memcpy(cmd.f, &c, sizeof(c));
    pushCmd(cmd);
}

void Renderer::strokeWidth(float size)
{
    DrawCmd cmd;
    cmd.type = DrawCmdType_strokeWidth;
    cmd.f[0] = size;
    pushCmd(cmd);
}

void Renderer::fontSize(float s)
{
    DrawCmd cmd;
    cmd.type = DrawCmdType_fontSize;
    cmd.f[0] = s;
    pushCmd(cmd);
}

void Renderer::fontBlur(float s)
{
    DrawCmd cmd;
    cmd.type = DrawCmdType_fontBlur;
    cmd.f[0] = s;
    pushCmd(cmd);
}

void Renderer::textAlign(int align)
{
    DrawCmd cmd;
    cmd.type = DrawCmdType_textAlign;
    cmd.integer = align;
    pushCmd(cmd);
}

void Renderer::text(float x, float y, const char *msg)
{
    DrawCmd cmd;
    cmd.type = DrawCmdType_text;
    cmd.f[0] = x;
    cmd.f[1] = y;
    strncpy(cmd.str, msg, sizeof(cmd.str));
    cmd.str[sizeof(cmd.str) - 1] = '\0';
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
    cmd.f[0] = x;
    cmd.f[1] = y;
    pushCmd(cmd);
}

void Renderer::lineTo(float x, float y)
{
    DrawCmd cmd;
    cmd.type = DrawCmdType_lineTo;
    cmd.f[0] = x;
    cmd.f[1] = y;
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
    cmd.f[0] = x;
    cmd.f[1] = y;
    cmd.f[2] = w;
    cmd.f[3] = h;
    cmd.f[4] = r;
    pushCmd(cmd);
}

void Renderer::rect(float x, float y, float w, float h)
{
    DrawCmd cmd;
    cmd.type = DrawCmdType_rect;
    cmd.f[0] = x;
    cmd.f[1] = y;
    cmd.f[2] = w;
    cmd.f[3] = h;
    pushCmd(cmd);
}

void Renderer::solidPathWinding(bool b)
{
    DrawCmd cmd;
    cmd.type = DrawCmdType_solidPathWinding;
    cmd.integer = b;
    pushCmd(cmd);
}

void Renderer::fillPaint(RenderPaint p)
{
    DrawCmd cmd;
    cmd.type = DrawCmdType_fillPaint;
    cmd.paint = p;
    pushCmd(cmd);
}

// Not rendering commands

static RenderPaint NVGpaintToRenderPaint(NVGpaint nvp)
{
    RenderPaint p = { 0 };

    assert(sizeof(p.xform) == sizeof(nvp.xform));
    assert(sizeof(p.extent) == sizeof(nvp.extent));

	memcpy(p.xform, nvp.xform, sizeof(p.xform));
	memcpy(p.extent, nvp.extent, sizeof(p.xform));
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
    RenderPaint p = { 0 };
    NVGpaint nvp = nvgLinearGradient(mNanoVg, sx, sy, ex, ey, nvgRGBAf(sc.r, sc.g, sc.b, sc.a), nvgRGBAf(ec.r, ec.g, ec.b, ec.a));
    return NVGpaintToRenderPaint(nvp);
}

RenderPaint Renderer::boxGradient(float x, float y, float w, float h,
                                  float r, float f, Color icol, Color ocol)
{
    NVGpaint nvp = nvgBoxGradient(mNanoVg, x, y, w, h, r, f, nvgRGBAf(icol.r, icol.g, icol.b, icol.a), nvgRGBAf(ocol.r, ocol.g, ocol.b, ocol.a));
    return NVGpaintToRenderPaint(nvp);
}

void Renderer::textBounds(int x, int y, const char *msg, float bounds[4])
{
    // TODO: Set current font settings before querying bounds
    nvgTextBounds(mNanoVg, x, y, msg, nullptr, bounds);
}

void Renderer::pushCmd(DrawCmd cmd)
{
    cmd.layer = mLayerStack.empty() ? 0 : mLayerStack.back();
    mDrawCmds.push_back(cmd);
}
