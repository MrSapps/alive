#ifndef ALIVE_RENDERER_HPP
#define ALIVE_RENDERER_HPP

#include <GL/glew.h>
#include "SDL_opengl.h"

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

struct Color {
    float r, g, b, a;
};

struct RenderPaint {
	float xform[6];
	float extent[2];
	float radius;
	float feather;
    Color innerColor;
    Color outerColor;
	int image;
};

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

enum PixelFormat
{
    PixelFormat_RGB24 = 0
};

// GLES2 renderer
class Renderer {
public:
    
    Renderer(const char *fontPath);
    ~Renderer();

    void beginFrame(int w, int h);
    void endFrame();

    // All coordinates are given in pixels.
    // All color components are given in 0..1 floating point.

    // Textured quad rendering
    int createTexture(void *pixels, int width, int height, PixelFormat format);
    void destroyTexture(int handle);
    void drawQuad(int texHandle, float x, float y, float w, float h);

    // NanoVG wrap
    void fillColor(Color c);
    void strokeColor(Color c);
    void strokeWidth(float size);
    void fontSize(float s);
    void textAlign(int align); // TextAlign bitfield
    void textBounds(int x, int y, const char *msg, float bounds[4]);
    void text(float x, float y, const char *msg);
    void resetTransform();
    void beginPath();
    void moveTo(float x, float y);
    void lineTo(float x, float y);
    void closePath();
    void fill();
    void stroke();
    void roundedRect(float x, float y, float w, float h, float r);

    RenderPaint linearGradient(float sx, float sy, float ex, float ey, Color sc, Color ec);
    void fillPaint(RenderPaint p);

private:
    void preVectorDraw();
    // Vector rendering
    struct NVGLUframebuffer* mNanoVgFrameBuffer = nullptr;
    struct NVGcontext* mNanoVg = nullptr;
    int mW, mH;

    // Textured quad rendering
    GLuint mVs;
    GLuint mFs;
    GLuint mProgram;
    Vao mQuadVao;

    enum Mode
    {
        Mode_none,
        Mode_vector,
        Mode_quads,
    };
    Mode mMode;
};

#endif