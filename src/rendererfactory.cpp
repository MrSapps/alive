#include "rendererfactory.hpp"
#include "openglrenderer.hpp"
#ifdef _MSC_VER
#include "directx9renderer.hpp"
#endif

/*static*/ std::unique_ptr<AbstractRenderer> RendererFactory::Create(SDL_Window* window, bool tryDirectX9)
{
#ifdef _MSC_VER
    if (tryDirectX9)
    {
        try
        {
            return std::make_unique<DirectX9Renderer>(window);
        }
        catch (const std::exception& e)
        {
            LOG_WARNING("Failed to start DirectX9 renderer: " << e.what() << ", trying OpenGL");
            return std::make_unique<OpenGLRenderer>(window);
        }
    }
    else
    {
        try
        {
            return std::make_unique<OpenGLRenderer>(window);
        }
        catch (const std::exception& e)
        {
            LOG_WARNING("Failed to start OpenGL renderer: " << e.what() << ", trying DirectX9");
            return std::make_unique<DirectX9Renderer>(window);
        }
    }

#else
    if (tryDirectX9)
    {
        LOG_WARNING("DirectX9 not supported on this platform");
    }
    return std::make_unique<OpenGLRenderer>(window);
#endif
}
