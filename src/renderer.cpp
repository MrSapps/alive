#include "renderer.hpp"
#include "oddlib/exceptions.hpp"
#include "logger.hpp"
#include "nanovg.h"
#define NANOVG_GLES3_IMPLEMENTATION
#include "nanovg_gl.h"
#include "nanovg_gl_utils.h"

#include <cassert>
#include "SDL_pixels.h""

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
            char* infoLog = (char*)malloc(sizeof(char) * infoLen);
            glGetShaderInfoLog(shader, infoLen, NULL, infoLog);
            LOG("Error compiling shader:\n%s\n", infoLog);
            free(infoLog);

            ALIVE_FATAL_ERROR();
        }
        return 0;
    }
    return shader;
}
Renderer::Renderer(const char *fontPath)
    : mMode(Mode_none)
{
    { // Vector rendering init
        LOG_INFO("Creating nanovg context");

#ifdef _DEBUG
        mNanoVg = nvgCreateGLES3(NVG_ANTIALIAS | NVG_STENCIL_STROKES | NVG_DEBUG);
#else
        mNanoVg = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES);
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
                char* infoLog = (char*)malloc(sizeof(char) * infoLen);
                glGetProgramInfoLog(mProgram, infoLen, NULL, infoLog);
                LOG("Error linking program:\n%s\n", infoLog);
                free(infoLog);
            }
            ALIVE_FATAL_ERROR();
        }

        mQuadVao = create_vao(4, 6);
    }
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
            nvgDeleteGLES3(mNanoVg);
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
    // Clear screen
    glClearColor(0.4f, 0.4f, 0.4f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    mMode = Mode_none;
    
    mW = w;
    mH = h;
}

void Renderer::endFrame()
{
    nvgResetTransform(mNanoVg);
    if (mMode == Mode_vector)
        nvgEndFrame(mNanoVg);
    mMode = Mode_none;

    GLenum error = glGetError();
    if (error != GL_NO_ERROR)
    {
        LOG_ERROR(gluErrorString(error));
    }
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
    if (mMode == Mode_vector)
        nvgEndFrame(mNanoVg);

    if (mMode != Mode_quads)
    {
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_STENCIL_TEST);
        glDisable(GL_CULL_FACE);
        glDisable(GL_SCISSOR_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    mMode = Mode_quads;

    float white[4] = { 1, 1, 1, 1 };

    bind_vao(&mQuadVao);

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

    vert[2].pos[0] = 2*(x + w) / mW - 1;
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
    reset_vao_mesh(&mQuadVao);
    add_vertices_to_vao(&mQuadVao, vert, 4);
    add_indices_to_vao(&mQuadVao, ind, 6);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texHandle);
    glUseProgram(mProgram);
    draw_vao(&mQuadVao);

    unbind_vao();
}

void Renderer::preVectorDraw()
{
    if (mMode != Mode_vector)
        nvgBeginFrame(mNanoVg, mW, mH, 1.0f);
    mMode = Mode_vector;
}

void Renderer::fillColor(Color c)
{
    preVectorDraw();
    nvgFillColor(mNanoVg, nvgRGBAf(c.r, c.g, c.b, c.a));
}

void Renderer::strokeColor(Color c)
{
    preVectorDraw();
    nvgStrokeColor(mNanoVg, nvgRGBAf(c.r, c.g, c.b, c.a));
}

void Renderer::strokeWidth(float size)
{
    preVectorDraw();
    nvgStrokeWidth(mNanoVg, size);
}

void Renderer::fontSize(float s)
{
    preVectorDraw();
    nvgFontSize(mNanoVg, s);
}

void Renderer::textAlign(int align)
{
    preVectorDraw();
    nvgTextAlign(mNanoVg, align);
}

void Renderer::textBounds(int x, int y, const char *msg, float bounds[4])
{
    preVectorDraw();
    nvgTextBounds(mNanoVg, x, y, msg, nullptr, bounds);
}

void Renderer::text(float x, float y, const char *msg)
{
    preVectorDraw();
    nvgText(mNanoVg, x, y, msg, nullptr);
}

void Renderer::resetTransform()
{
    preVectorDraw();
    nvgResetTransform(mNanoVg);
}

void Renderer::beginPath()
{
    preVectorDraw();
    nvgBeginPath(mNanoVg);
}

void Renderer::moveTo(float x, float y)
{
    preVectorDraw();
    nvgMoveTo(mNanoVg, x, y);
}

void Renderer::lineTo(float x, float y)
{
    preVectorDraw();
    nvgLineTo(mNanoVg, x, y);
}

void Renderer::closePath()
{
    preVectorDraw();
    nvgClosePath(mNanoVg);
}

void Renderer::fill()
{
    preVectorDraw();
    nvgFill(mNanoVg);
}

void Renderer::stroke()
{
    preVectorDraw();
    nvgStroke(mNanoVg);
}

void Renderer::roundedRect(float x, float y, float w, float h, float r)
{
    preVectorDraw();
    nvgRoundedRect(mNanoVg, x, y, w, h, r);
}

RenderPaint Renderer::linearGradient(float sx, float sy, float ex, float ey, Color sc, Color ec)
{
    preVectorDraw();

    RenderPaint p = { 0 };
    NVGpaint nvp = nvgLinearGradient(mNanoVg, sx, sy, ex, ey, nvgRGBAf(sc.r, sc.g, sc.b, sc.a), nvgRGBAf(ec.r, ec.g, ec.b, ec.a));

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

void Renderer::fillPaint(RenderPaint p)
{
    preVectorDraw();

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
}
