#ifndef ALIVE_RENDERER_HPP
#define ALIVE_RENDERER_HPP

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

// GLES2 renderer
class Renderer {
public:
    
    Renderer(const char *fontPath);
    ~Renderer();

    void beginFrame(int w, int h);
    void endFrame();

    // NanoVG wrap
    void drawText(int xpos, int ypos, const char *msg);
    void fillColor(int r, int g, int b, int a);
    void fontSize(float s);
    void textAlign(TextAlign align);
    void textBounds(int x, int y, const char *msg, float bounds[4]);

private:
    struct NVGLUframebuffer* mNanoVgFrameBuffer = nullptr;
    struct NVGcontext* mNanoVg = nullptr;
};

#endif