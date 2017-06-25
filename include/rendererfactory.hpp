#pragma once

#include <memory>
#include "SDL.h"

class AbstractRenderer;

class RendererFactory
{
public:
    static std::unique_ptr<AbstractRenderer> Create(SDL_Window* window, bool tryDirectX9);
};
