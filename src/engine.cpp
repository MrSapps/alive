#include "engine.hpp"
#include <iostream>
#include "oddlib/lvlarchive.hpp"
#include "oddlib/masher.hpp"
#include "oddlib/exceptions.hpp"
#include "logger.hpp"
#include "imgui/imgui.h"
#include "imgui/stb_rect_pack.h"
#include "jsonxx/jsonxx.h"
#include <fstream>
#include "alive_version.h"
#include "core/audiobuffer.hpp"

extern "C"
{
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}



#ifdef _WIN32
#include <windows.h>
#include "../rsc/resource.h"
#include "SDL_syswm.h"
#include <functional>


void setWindowsIcon(SDL_Window *sdlWindow)
{
    HINSTANCE handle = ::GetModuleHandle(nullptr);
    HICON icon = ::LoadIcon(handle, MAKEINTRESOURCE(IDI_MAIN_ICON));
    if (icon != nullptr)
    {
        SDL_SysWMinfo wminfo;
        SDL_VERSION(&wminfo.version);
        if (SDL_GetWindowWMInfo(sdlWindow, &wminfo) == 1)
        {
            HWND hwnd = wminfo.info.win.window;
#ifdef _WIN64
            ::SetClassLongPtr(hwnd, GCLP_HICON, reinterpret_cast<LONG_PTR>(icon));
#else
            ::SetClassLong(hwnd, GCL_HICON, reinterpret_cast<LONG>(icon));
#endif
        }

        HMODULE hKernel32 = ::GetModuleHandle("Kernel32.dll");
        if (hKernel32)
        {
            typedef BOOL(WINAPI *pSetConsoleIcon)(HICON icon);
            pSetConsoleIcon setConsoleIcon = (pSetConsoleIcon)::GetProcAddress(hKernel32, "SetConsoleIcon");
            if (setConsoleIcon)
            {
                setConsoleIcon(icon);
            }
        }
    }
}
#endif

static GLuint fontTex;
static bool mousePressed[4] = { false, false };
static ImVec2 mousePosScale(1.0f, 1.0f);

// This is the main rendering function that you have to implement and provide to ImGui (via setting up 'RenderDrawListsFn' in the ImGuiIO structure)
// If text or lines are blurry when integrating ImGui in your engine:
// - in your Render function, try translating your projection matrix by (0.5f,0.5f) or (0.375f,0.375f)
// - try adjusting ImGui::GetIO().PixelCenterOffset to 0.5f or 0.375f
static void ImImpl_RenderDrawLists(ImDrawList** const cmd_lists, int cmd_lists_count)
{
    if (cmd_lists_count == 0)
        return;

    // We are using the OpenGL fixed pipeline to make the example code simpler to read!
    // A probable faster way to render would be to collate all vertices from all cmd_lists into a single vertex buffer.
    // Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled, vertex/texcoord/color pointers.
    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_TRANSFORM_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glEnable(GL_TEXTURE_2D);
    //glUseProgram(0); // You may want this if using this code in an OpenGL 3+ context

    // Setup orthographic projection matrix
    const float width = ImGui::GetIO().DisplaySize.x;
    const float height = ImGui::GetIO().DisplaySize.y;

    //std::cout << "ON RENDER " << width << " " << height << std::endl;

    glViewport(0, 0, width, height);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0f, width, height, 0.0f, -1.0f, +1.0f);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // Render command lists
#define OFFSETOF(TYPE, ELEMENT) ((size_t)&(((TYPE *)0)->ELEMENT))
    for (int n = 0; n < cmd_lists_count; n++)
    {
        const ImDrawList* cmd_list = cmd_lists[n];
        const unsigned char* vtx_buffer = (const unsigned char*)&cmd_list->vtx_buffer.front();
        glVertexPointer(2, GL_FLOAT, sizeof(ImDrawVert), (void*)(vtx_buffer + OFFSETOF(ImDrawVert, pos)));
        glTexCoordPointer(2, GL_FLOAT, sizeof(ImDrawVert), (void*)(vtx_buffer + OFFSETOF(ImDrawVert, uv)));
        glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(ImDrawVert), (void*)(vtx_buffer + OFFSETOF(ImDrawVert, col)));

        int vtx_offset = 0;
        for (size_t cmd_i = 0; cmd_i < cmd_list->commands.size(); cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->commands[cmd_i];
            if (pcmd->user_callback)
            {
                pcmd->user_callback(cmd_list, pcmd);
            }
            else
            {
                glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->texture_id);
                glScissor((int)pcmd->clip_rect.x, (int)(height - pcmd->clip_rect.w), (int)(pcmd->clip_rect.z - pcmd->clip_rect.x), (int)(pcmd->clip_rect.w - pcmd->clip_rect.y));
                glDrawArrays(GL_TRIANGLES, vtx_offset, pcmd->vtx_count);
            }
            vtx_offset += pcmd->vtx_count;
        }
    }
#undef OFFSETOF

    // Restore modified state
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glBindTexture(GL_TEXTURE_2D, 0);
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glPopAttrib();
}

static GLuint       g_FontTexture = 0;

void Engine::ImGui_WindowResize()
{
    int w, h;
    int fb_w, fb_h;
    SDL_GetWindowSize(window, &w, &h);
    SDL_GetWindowSize(window, &fb_w, &fb_h); // Needs to be corrected for SDL Framebuffer
    mousePosScale.x = (float)fb_w / w;
    mousePosScale.y = (float)fb_h / h;

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)fb_w, (float)fb_h);  // Display size, in pixels. For clamping windows positions.

    std::cout << "ON RESIZE " << io.DisplaySize.x << " " << io.DisplaySize.y << std::endl;


    //    io.PixelCenterOffset = 0.0f;                        // Align OpenGL texels

}

void UpdateImGui()
{
    ImGuiIO& io = ImGui::GetIO();

    SDL_PumpEvents();

    // Setup inputs
    // (we already got mouse wheel, keyboard keys & characters from glfw callbacks polled in glfwPollEvents())
    int mouse_x, mouse_y;
    SDL_GetMouseState(&mouse_x, &mouse_y);

    io.MousePos = ImVec2((float)mouse_x * mousePosScale.x, (float)mouse_y * mousePosScale.y);      // Mouse position, in pixels (set to -1,-1 if no mouse / on another screen, etc.)

    io.MouseDown[0] = mousePressed[0] || (SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_LEFT));
    io.MouseDown[1] = mousePressed[1] || (SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_RIGHT));

    // Start the frame
    ImGui::NewFrame();
}

Engine::Engine()
{

}

Engine::~Engine()
{

    if (g_FontTexture)
    {
        glDeleteTextures(1, &g_FontTexture);
        ImGui::GetIO().Fonts->TexID = 0;
        g_FontTexture = 0;
    }

    ImGui::Shutdown();
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

bool Engine::Init()
{
    if (!InitSDL())
    {
        LOG_ERROR("SDL init failure");
        return false;
    }

    if (!mFileSystem.Init())
    {
        LOG_ERROR("File system init failure");
        return false;
    }

    if (!mGameData.Init(mFileSystem.BasePath())) // TODO
    {
        LOG_ERROR("Game data init failure");
        return false;
    }

    InitGL();
    InitImGui();

    return true;
}

int Engine::Run()
{
    while (mRunning)
    {
        Update();
        Render();
    }
    return 0;
}

void Engine::Update()
{
    ImGuiIO& io = ImGui::GetIO();
    mousePressed[0] = mousePressed[1] = false;
    io.MouseWheel = 0;

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_MOUSEWHEEL:
            if (event.wheel.y < 0)
            {
                ImGui::GetIO().MouseWheel = -1.0f;
            }
            else
            {
                ImGui::GetIO().MouseWheel = 1.0f;
            }
            break;

        case SDL_QUIT:
            mRunning = false;
            break;

        case SDL_TEXTINPUT:
        {
            size_t len = strlen(event.text.text);
            for (size_t i = 0; i < len; i++)
            {
                uint32_t keycode = event.text.text[i];
                if (keycode >= 32 && keycode <= 255)
                {
                    //printable ASCII characters
                    ImGui::GetIO().AddInputCharacter((char)keycode);
                }
            }
        }
            break;

        case SDL_WINDOWEVENT:
            switch (event.window.event)
            {
            case SDL_WINDOWEVENT_RESIZED:
            case SDL_WINDOWEVENT_MAXIMIZED:
            case SDL_WINDOWEVENT_RESTORED:
                ImGui_WindowResize();

                break;
            }
            break;

        case SDL_KEYDOWN:
        case SDL_KEYUP:
        {
            if (event.type == SDL_KEYDOWN)
            {
                if (event.key.keysym.sym == 13)
                {
                    const Uint32 windowFlags = SDL_GetWindowFlags(window);
                    bool isFullScreen = ((windowFlags & SDL_WINDOW_FULLSCREEN_DESKTOP) || (windowFlags & SDL_WINDOW_FULLSCREEN));
                    SDL_SetWindowFullscreen(window, isFullScreen ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
                    ImGui_WindowResize();
                }

            }

            const SDL_Scancode key = SDL_GetScancodeFromKey(event.key.keysym.sym);
            if (key == SDL_SCANCODE_ESCAPE)
            {

                SDL_ShowCursor(1);
                video = nullptr;
                if (videoFrame)
                {
                    SDL_FreeSurface(videoFrame);
                    videoFrame = nullptr;
                }
                //targetFps = 60;

            }

            if (key >= 0 && key < 512)
            {
                SDL_Keymod modstate = SDL_GetModState();

                ImGuiIO& io = ImGui::GetIO();
                io.KeysDown[key] = event.type == SDL_KEYDOWN;
                io.KeyCtrl = (modstate & KMOD_CTRL) != 0;
                io.KeyShift = (modstate & KMOD_SHIFT) != 0;
            }
        }
            break;
        }
    }

    UpdateImGui();
}

void Engine::DrawVideoSelectionUi(const std::string& setName, const std::vector<std::string>& allFmvs)
{
    std::string name = "Video player (" + setName + ")";
    ImGui::Begin(name.c_str(), nullptr, ImVec2(550, 580), 1.0f, ImGuiWindowFlags_NoCollapse);
#ifdef _WIN32
    static char buf[4096] = "C:\\Program Files (x86)\\Steam\\SteamApps\\common\\Oddworld Abes Exoddus\\";
#else
    static char buf[4096] = "/home/paul/ae_test/";
#endif

    ImGui::InputText("Video path", buf, sizeof(buf));

    static ImGuiTextFilter filter;
    filter.Draw();


    static int listbox_item_current = 1;
    static std::vector<const char*> listbox_items;
    listbox_items.resize(allFmvs.size());

    int matchingFilter = 0;
    for (size_t i = 0; i < allFmvs.size(); i++)
    {
        if (filter.PassFilter(allFmvs[i].c_str()))
        {
            listbox_items[matchingFilter] = allFmvs[i].c_str();
            matchingFilter++;
        }
    }
    ImGui::PushItemWidth(-1);
    ImGui::ListBox("##", &listbox_item_current, listbox_items.data(), matchingFilter, 27);

    if (ImGui::Button("Play", ImVec2(ImGui::GetWindowWidth(), 20)))
    {
        std::string fullPath = std::string(buf) + listbox_items[listbox_item_current];
        std::cout << "Play " << listbox_items[listbox_item_current] << std::endl;
        try
        {

            video = std::make_unique<Oddlib::Masher>(fullPath);
            if (videoFrame)
            {
                SDL_FreeSurface(videoFrame);
                videoFrame = nullptr;
            }

            if (video->HasAudio())
            {
                AudioBuffer::ChangeAudioSpec(video->SingleAudioFrameSizeBytes(), video->AudioSampleRate());
            }

            if (video->HasVideo())
            {
                videoFrame = SDL_CreateRGBSurface(0, video->Width(), video->Height(), 32, 0, 0, 0, 0);
                //                    targetFps = video->FrameRate() * 2;
            }
            SDL_ShowCursor(0);
        }
        catch (const Oddlib::Exception& ex)
        {
            // ImGui::Text(ex.what());
        }
    }

    ImGui::End();
}

void Engine::RenderVideoUi()
{
    
    if (!video)
    {
        auto fmvs = mGameData.Fmvs();
        for (auto fmvSet : fmvs)
        {
            DrawVideoSelectionUi(fmvSet.first, fmvSet.second);
        }
    }
    else
    {
        std::vector<Uint16> decodedFrame(video->SingleAudioFrameSizeBytes() * 2); // *2 if stereo

        if (!video->Update((Uint32*)videoFrame->pixels, (Uint8*)decodedFrame.data()))
        {
            SDL_ShowCursor(1);
            video = nullptr;
            if (videoFrame)
            {
                SDL_FreeSurface(videoFrame);
                videoFrame = nullptr;
            }
            //targetFps = 60;
        }
        else
        {
            AudioBuffer::SendSamples((char*)decodedFrame.data(), decodedFrame.size() * 2);
            while (AudioBuffer::mPlayedSamples < video->FrameNumber() * video->SingleAudioFrameSizeBytes())
            {

            }

        }
    }
}

void Engine::Render()
{
    // Render main menu bar
    static bool showAbout = false;
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Exit", "CTRL+Q")) { mRunning = false; }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit"))
        {
            if (ImGui::MenuItem("Undo", "CTRL+Z")) {}
            if (ImGui::MenuItem("Redo", "CTRL+Y", false, false)) {}  // Disabled item
            ImGui::Separator();
            if (ImGui::MenuItem("Cut", "CTRL+X")) {}
            if (ImGui::MenuItem("Copy", "CTRL+C")) {}
            if (ImGui::MenuItem("Paste", "CTRL+V")) {}
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help"))
        {
            if (ImGui::MenuItem("About", "CTRL+H"))
            {
                showAbout = !showAbout;
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    //ImGui::ShowTestWindow();

    // Render about screen
    if (showAbout)
    {
        ImGui::Begin(ALIVE_VERSION_NAME_STR, nullptr, ImVec2(400, 100));
        ImGui::Text("Open source ALIVE engine PaulsApps.com 2015");
        ImGui::End();
    }

    RenderVideoUi();

    // Clear screen
    glClearColor(0.6f, 0.6f, 0.6f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Draw UI buffers
    ImGui::Render();

    GLenum error = glGetError();
    if (error != GL_NO_ERROR)
    {
        //            std::cout << gluErrorString(error) << std::endl;
    }

    glEnable(GL_TEXTURE_2D);

    
    if (videoFrame)
    {
        // TODO: Optimize - should use VBO's & update 1 texture rather than creating per frame
        GLuint TextureID = 0;

        glGenTextures(1, &TextureID);
        glBindTexture(GL_TEXTURE_2D, TextureID);

        int Mode = GL_RGB;

        if (videoFrame->format->BytesPerPixel == 4) {
            Mode = GL_RGBA;
        }

        glTexImage2D(GL_TEXTURE_2D, 0, Mode, videoFrame->w, videoFrame->h, 0, Mode, GL_UNSIGNED_BYTE, videoFrame->pixels);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


        // For Ortho mode, of course
        int X = -1;
        int Y = -1;
        int Width = 2;
        int Height = 2;

        glBegin(GL_QUADS);
        glTexCoord2f(0, 0); glVertex3f(X, Y, 0);
        glTexCoord2f(1, 0); glVertex3f(X + Width, Y, 0);
        glTexCoord2f(1, -1); glVertex3f(X + Width, Y + Height, 0);
        glTexCoord2f(0, -1); glVertex3f(X, Y + Height, 0);
        glEnd();

        glDeleteTextures(1, &TextureID);
    }
    

    // Flip the buffers
    SDL_GL_SwapWindow(window);
}

bool Engine::InitSDL()
{
    SDL_Init(SDL_INIT_EVERYTHING);

    window = SDL_CreateWindow(ALIVE_VERSION_NAME_STR,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

#if defined(_WIN32)
    // I'd like my icon back thanks
    setWindowsIcon(window);
#endif

    return true;
}

void Engine::InitImGui()
{
    ImGuiIO& io = ImGui::GetIO();
    ImGui_WindowResize();
    io.RenderDrawListsFn = ImImpl_RenderDrawLists;

    // Build texture
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsAlpha8(&pixels, &width, &height);

    // Create texture
    glGenTextures(1, &g_FontTexture);
    glBindTexture(GL_TEXTURE_2D, g_FontTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, width, height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, pixels);

    // Store our identifier
    io.Fonts->TexID = (void *)(intptr_t)g_FontTexture;

    // Cleanup (don't clear the input data if you want to append new fonts later)
    io.Fonts->ClearInputData();
    io.Fonts->ClearTexData();

    //
    // setup SDL2 keymapping
    //
    io.KeyMap[ImGuiKey_Tab] = SDL_GetScancodeFromKey(SDLK_TAB);
    io.KeyMap[ImGuiKey_LeftArrow] = SDL_GetScancodeFromKey(SDLK_LEFT);
    io.KeyMap[ImGuiKey_RightArrow] = SDL_GetScancodeFromKey(SDLK_RIGHT);
    io.KeyMap[ImGuiKey_UpArrow] = SDL_GetScancodeFromKey(SDLK_UP);
    io.KeyMap[ImGuiKey_DownArrow] = SDL_GetScancodeFromKey(SDLK_DOWN);
    io.KeyMap[ImGuiKey_Home] = SDL_GetScancodeFromKey(SDLK_HOME);
    io.KeyMap[ImGuiKey_End] = SDL_GetScancodeFromKey(SDLK_END);
    io.KeyMap[ImGuiKey_Delete] = SDL_GetScancodeFromKey(SDLK_DELETE);
    io.KeyMap[ImGuiKey_Backspace] = SDL_GetScancodeFromKey(SDLK_BACKSPACE);
    io.KeyMap[ImGuiKey_Enter] = SDL_GetScancodeFromKey(SDLK_RETURN);
    io.KeyMap[ImGuiKey_Escape] = SDL_GetScancodeFromKey(SDLK_ESCAPE);
    io.KeyMap[ImGuiKey_A] = SDLK_a;
    io.KeyMap[ImGuiKey_C] = SDLK_c;
    io.KeyMap[ImGuiKey_V] = SDLK_v;
    io.KeyMap[ImGuiKey_X] = SDLK_x;
    io.KeyMap[ImGuiKey_Y] = SDLK_y;
    io.KeyMap[ImGuiKey_Z] = SDLK_z;

}

void Engine::InitGL()
{
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 32);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
        SDL_GL_CONTEXT_PROFILE_CORE);



    context = SDL_GL_CreateContext(window);

    //    glewExperimental = GL_TRUE;
    // GLenum status = glewInit();
    // if (status != GLEW_OK) {
    //     printf("Could not initialize GLEW!\n");
    //  }
}
