#include "SDL.h"
#include <iostream>
#include "oddlib/lvlarchive.hpp"

int main(int argc, char** argv)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_HAPTIC) != 0)
    {
        std::cout << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_Haptic *haptic;

    // Open the device
    haptic = SDL_HapticOpen(0);
    if (haptic == NULL)
        return -1;

    // Initialize simple rumble
    if (SDL_HapticRumbleInit(haptic) != 0)
        return -1;

    // Play effect at 50% strength for 2 seconds
     if (SDL_HapticRumblePlay(haptic, 1.0, 2000) != 0)
        return -1;
    SDL_Delay(2000);

    // Clean up
    SDL_HapticClose(haptic);

    SDL_Window *win = SDL_CreateWindow("A.L.I.V.E", 100, 100, 640, 480, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (win == nullptr)
    {
        std::cout << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (ren == nullptr)
    {
        SDL_DestroyWindow(win);
        std::cout << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    std::string imagePath = "test.bmp";
    SDL_Surface *bmp = SDL_LoadBMP(imagePath.c_str());
    if (bmp == nullptr)
    {
        SDL_DestroyRenderer(ren);
        SDL_DestroyWindow(win);
        std::cout << "SDL_LoadBMP Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SDL_Texture *tex = SDL_CreateTextureFromSurface(ren, bmp);
    SDL_FreeSurface(bmp);
    if (tex == nullptr)
    {
        SDL_DestroyRenderer(ren);
        SDL_DestroyWindow(win);
        std::cout << "SDL_CreateTextureFromSurface Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SDL_RenderClear(ren);
    SDL_RenderCopy(ren, tex, NULL, &bmp->clip_rect);
    SDL_RenderPresent(ren);

    const Uint32 fps = 60;
    const Uint32 minframetime = 1000 / fps;

    bool running = true;
    SDL_Event event;
    Uint32 frametime;

    while (running)
    {

        frametime = SDL_GetTicks();

        while (SDL_PollEvent(&event) != 0)
        {
            switch (event.type)
            {
            case SDL_KEYDOWN: 
                if (event.key.keysym.sym == SDLK_ESCAPE)
                {
                    running = false;
                }
                break;
            }
        }

        if (SDL_GetTicks() - frametime < minframetime)
            SDL_Delay(minframetime - (SDL_GetTicks() - frametime));

    }

    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();

    return 0;
}