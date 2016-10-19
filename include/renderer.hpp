#pragma once

#include "types.hpp"
#include <vector>

#include <GL/gl3w.h>
#ifdef WIN32_LEAN_AND_MEAN
#undef WIN32_LEAN_AND_MEAN
#endif
#include "SDL_opengl.h"

#include <glm/glm.hpp>
#include <glm/vec3.hpp> // glm::vec3
#include <glm/vec4.hpp> // glm::vec4
#include <glm/mat4x4.hpp> // glm::mat4
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/compatibility.hpp>
#include "logger.hpp"
#include <memory>

// Vertex array object. Contains vertex and index buffers
// Copy-pasted from revolc engine
typedef struct Vao {
    GLuint vao_id;
    GLuint vbo_id;
    GLuint ibo_id;

    int v_count;
    int v_capacity;
    int v_size;
    int i_count;
    int i_capacity;
} Vao;
typedef uint32_t MeshIndexType;
#define MESH_INDEX_GL_TYPE GL_UNSIGNED_INT

Vao create_vao(int max_v_count, int max_i_count);
void destroy_vao(Vao *vao);
void bind_vao(const Vao *vao);
void unbind_vao();
void add_vertices_to_vao(Vao *vao, void *vertices, int count);
void add_indices_to_vao(Vao *vao, MeshIndexType *indices, int count);
void reset_vao_mesh(Vao *vao);
void draw_vao(const Vao *vao);


// These values should match nanovg
enum TextAlign {
    TEXT_ALIGN_LEFT 	= 1<<0,	// Default, align text horizontally to left.
    TEXT_ALIGN_CENTER 	= 1<<1,	// Align text horizontally to center.
    TEXT_ALIGN_RIGHT 	= 1<<2,	// Align text horizontally to right.
    TEXT_ALIGN_TOP 		= 1<<3,	// Align text vertically to top.
    TEXT_ALIGN_MIDDLE	= 1<<4,	// Align text vertically to middle.
    TEXT_ALIGN_BOTTOM	= 1<<5,	// Align text vertically to bottom. 
    TEXT_ALIGN_BASELINE	= 1<<6, // Default, align text vertically to baseline. 
};

struct ColourF32 
{
    f32 r, g, b, a;
    static ColourF32 white();
};

struct ColourU8
{
    u8 r, g, b, a;
    constexpr ColourF32 ToColourF32() const
    {
        return { r / 255.0f, g / 255.0f, b/255.0f, a / 255.0f };
    }
};


// For NanoVG wrap
struct RenderPaint {
    f32 xform[6];
    f32 extent[2];
    f32 radius;
    f32 feather;
    ColourF32 innerColor;
    ColourF32 outerColor;
    int image;
};

struct BlendMode {
    GLenum srcFactor;
    GLenum dstFactor;
    GLenum equation;
    f32 colorMul;

    // Some usual blend modes
    static BlendMode normal();
    static BlendMode additive();
    static BlendMode subtractive();
    static BlendMode opaque();
    static BlendMode B100F100();
};

// Internal to Renderer
enum DrawCmdType {
    DrawCmdType_quad,
    DrawCmdType_fillColor,
    DrawCmdType_strokeColor,
    DrawCmdType_strokeWidth,
    DrawCmdType_fontSize,
    DrawCmdType_fontBlur,
    DrawCmdType_textAlign,
    DrawCmdType_text,
    DrawCmdType_resetTransform,
    DrawCmdType_beginPath,
    DrawCmdType_moveTo,
    DrawCmdType_lineTo,
    DrawCmdType_closePath,
    DrawCmdType_fill,
    DrawCmdType_stroke,
    DrawCmdType_roundedRect,
    DrawCmdType_rect,
    DrawCmdType_circle,
    DrawCmdType_solidPathWinding,
    DrawCmdType_fillPaint,
    DrawCmdType_scissor,
    DrawCmdType_resetScissor,
    DrawCmdType_lineCap,
    DrawCmdType_lineJoin
};

struct DrawCmd {
    DrawCmdType type;
    int layer;
    union { // This union is to make the struct smaller, as RenderPaint is quite large
        struct {
            int integer;
            f32 f[5];
            BlendMode blendMode; // Used only for quads, for now.
            ColourF32 color;
            char str[128]; // TODO: Allocate dynamically from cheap frame allocator
        } s;
        RenderPaint paint;
    };
};

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

// GLES2 renderer
class Renderer {
public:
    
    Renderer(const char *fontPath);
    ~Renderer();

    glm::vec2 mScreenSize = glm::vec2(368, 240);
    glm::vec2 mCameraPosition = glm::vec2(0, 0);
    bool mSmoothCameraPosition = false;

    void beginFrame(int w, int h);
    void endFrame();

    // Layers with larger depth are drawn on top
    void beginLayer(int depth);
    void endLayer();

    int createTexture(GLenum internalFormat, int width, int height, GLenum inputFormat, GLenum colorDataType, const void *pixels, bool interpolation);
    void destroyTexture(int handle);

    // Drawing commands, which will be buffered and issued at the end of the frame.
    // All coordinates are given in pixels.
    // All color components are given in 0..1 floating point.

    // Use negative w or h to flip uv coordinates
    void drawQuad(int texHandle, f32 x, f32 y, f32 w, f32 h, ColourF32 color = ColourF32::white(), BlendMode blendMode = BlendMode::normal());

    glm::vec2 WorldToScreen(const glm::vec2& worldPos)
    {
        return ((mCameraProjection * mCameraView) * glm::vec4(worldPos, 1, 1)) * glm::vec4(mW / 2, -mH / 2, 1, 1) + glm::vec4(mW / 2, mH / 2, 0, 0);
    }

    glm::vec4 WorldToScreenRect(f32 x, f32 y, f32 width, f32 height)
    {
        glm::vec2 rectPos = glm::vec2(x, y);
        glm::vec2 rectSize = glm::vec2(width, height);
        glm::vec2 screenRectPos = WorldToScreen(rectPos);
        glm::vec2 screenRectSize = WorldToScreen(rectPos + rectSize) - screenRectPos;

        return glm::vec4(screenRectPos, screenRectSize);
    }

    // NanoVG wrap
    void fillColor(ColourF32 c);
    void strokeColor(ColourF32 c);
    void strokeWidth(f32 size);
    void fontSize(f32 s);
    void fontBlur(f32 s);
    void textAlign(int align); // TextAlign bitfield
    void text(f32 x, f32 y, const char *msg);
    void resetTransform();
    void beginPath();
    void moveTo(f32 x, f32 y);
    void lineTo(f32 x, f32 y);
    void closePath();
    void fill();
    void stroke();
    void roundedRect(f32 x, f32 y, f32 w, f32 h, f32 r);
    void rect(f32 x, f32 y, f32 w, f32 h);
    void circle(f32 x, f32 y, f32 r);
    void solidPathWinding(bool b); // If false, then holes are created
    void fillPaint(RenderPaint p);
    void scissor(f32 x, f32 y, f32 w, f32 h);
    void resetScissor();
    void lineCap(int cap);
    void LineJoin(int cap);

    // Not drawing commands
    RenderPaint linearGradient(f32 sx, f32 sy, f32 ex, f32 ey, ColourF32 sc, ColourF32 ec);
    RenderPaint boxGradient(f32 x, f32 y, f32 w, f32 h,
                            f32 r, f32 f, ColourF32 icol, ColourF32 ocol);
    RenderPaint radialGradient(f32 cx, f32 cy, f32 inr, f32 outr, ColourF32 icol, ColourF32 ocol);
    // TODO: Add fontsize param to make independent of "current state"
    void textBounds(int x, int y, const char *msg, f32 bounds[4]);

    int mW = 0;
    int mH = 0;
    void updateCamera();

private:
    void destroyTextures();
    void pushCmd(DrawCmd cmd);

    // Vector rendering
    struct NVGLUframebuffer* mNanoVgFrameBuffer = nullptr;
    struct NVGcontext* mNanoVg = nullptr;

    // Textured quad rendering
    GLuint mVs;
    GLuint mFs;
    GLuint mProgram;
    Vao mQuadVao;

    glm::mat4 mView;
    glm::mat4 mCameraProjection;
    glm::mat4 mCameraView;

    enum Mode
    {
        Mode_none,
        Mode_vector,
        Mode_quads,
    };

    std::vector<int> mLayerStack;
    std::vector<DrawCmd> mDrawCmds;
    std::vector<int> mDestroyTextureList;
};
