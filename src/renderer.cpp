#include "renderer.hpp"

#include <GL/glew.h>
#include "SDL_opengl.h"

#include "oddlib/exceptions.hpp"
#include "logger.hpp"
#include "nanovg.h"
#define NANOVG_GLES3_IMPLEMENTATION
#include "nanovg_gl.h"
#include "nanovg_gl_utils.h"

static GLuint fontTex;
static GLuint       g_FontTexture = 0;

Renderer::Renderer(const char *fontPath)
{
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
    mNanoVgFrameBuffer = nvgluCreateFramebuffer(mNanoVg, 800*2, 600*2, 0);
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

Renderer::~Renderer()
{
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

void Renderer::beginFrame(int w, int h)
{
    // Clear screen
    glClearColor(0.6f, 0.6f, 0.6f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    
    // Scaled vector rendering area
    nvgBeginFrame(mNanoVg, w, h, 1.0f);
}

void Renderer::endFrame()
{
    nvgResetTransform(mNanoVg);

    GLenum error = glGetError();
    if (error != GL_NO_ERROR)
    {
        LOG_ERROR(gluErrorString(error));
    }

    nvgEndFrame(mNanoVg);

    // Draw UI buffers
    //ImGui::Render();

    // Flip the buffers
    nvgluBindFramebuffer(nullptr);
}

void Renderer::drawText(int xpos, int ypos, const char *msg)
{
    nvgText(mNanoVg, xpos, ypos, msg, nullptr);
}

void Renderer::fillColor(int r, int g, int b, int a)
{
    nvgFillColor(mNanoVg, nvgRGBA(r, g, b, a));
}

void Renderer::fontSize(float s)
{
    nvgFontSize(mNanoVg, s);
}

void Renderer::textAlign(TextAlign align)
{
    nvgTextAlign(mNanoVg, align);
}

void Renderer::textBounds(int x, int y, const char *msg, float bounds[4])
{
    nvgTextBounds(mNanoVg, x, y, msg, nullptr, bounds);
}

