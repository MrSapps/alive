#include "SDL.h"
#include <iostream>
#include "oddlib/lvlarchive.hpp"
#include "imgui/imgui.h"
#include "imgui/stb_rect_pack.h"
#include "jsonxx/jsonxx.h"
#include <fstream>

extern "C"
{
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

//#include <GL/glew.h>
#ifdef _WIN32
#include <windows.h>
#endif
#ifdef __APPLE__
#  include <OpenGL/gl.h>
#else
#  include <GL/gl.h>
#endif/*__APPLE__*/

static SDL_Window* window;
static SDL_GLContext context;
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

void InitGL()
{
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 32);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
        SDL_GL_CONTEXT_PROFILE_CORE);

    SDL_Init(SDL_INIT_EVERYTHING);

    window = SDL_CreateWindow("SDL IMGui Example.",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720,
        SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN | SDL_WINDOW_FULLSCREEN_DESKTOP);
    context = SDL_GL_CreateContext(window);

//    glewExperimental = GL_TRUE;
   // GLenum status = glewInit();
   // if (status != GLEW_OK) {
   //     printf("Could not initialize GLEW!\n");
  //  }
}
static GLuint       g_FontTexture = 0;

void InitImGui()
{
    int w, h;
    int fb_w, fb_h;
    SDL_GetWindowSize(window, &w, &h);
    SDL_GetWindowSize(window, &fb_w, &fb_h); // Needs to be corrected for SDL Framebuffer
    mousePosScale.x = (float)fb_w / w;
    mousePosScale.y = (float)fb_h / h;

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)fb_w, (float)fb_h);  // Display size, in pixels. For clamping windows positions.
//    io.PixelCenterOffset = 0.0f;                        // Align OpenGL texels
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
// Application code
int main(int argc, char** argv)
{

    lua_State *L = lua_open();
    lua_close(L);

    InitGL();
    InitImGui();

    jsonxx::Object rootJsonObject;
    std::ifstream tmpStream("./data/videos.json");
    if (!tmpStream)
    {
        return 1;
    }
    std::string jsonFileContents((std::istreambuf_iterator<char>(tmpStream)),std::istreambuf_iterator<char>());

    std::vector<std::string> allFmvs;

    rootJsonObject.parse(jsonFileContents);
    if (rootJsonObject.has<jsonxx::Object>("FMVS"))
    {
        jsonxx::Object& fmvObj = rootJsonObject.get<jsonxx::Object>("FMVS");
        for (auto& v : fmvObj.kv_map())
        {
            jsonxx::Array& ar = fmvObj.get<jsonxx::Array>(v.first);
            for (size_t i = 0; i < ar.size(); i++)
            {
                jsonxx::String s = ar.get<jsonxx::String>(i);
                if (s.length() < 12)
                {
                    s.append(12 - s.length(), ' ');
                }
                allFmvs.emplace_back(s);

            }
        }

      //  for (auto& value : fmvsArray.values())
        {
           // jsonxx::Array& a = value->get<jsonxx::Array>();

        }
    }

    bool running = true;
    bool selected = false;
    while (running) {
        ImGuiIO& io = ImGui::GetIO();
        mousePressed[0] = mousePressed[1] = false;
        io.MouseWheel = 0;

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
        }

        UpdateImGui();

        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Exit", "CTRL+Q")) { running = false; }
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
            ImGui::EndMainMenuBar();
        }

        ImGui::Begin("Video player");
        for (auto& v : allFmvs)
        {
            if (ImGui::Button(v.c_str()))
            {
                std::cout << "Play " << v.c_str() << std::endl;
            }
            ImGui::Separator();
        }

       

        if (ImGui::Button("OK")) 
        {
            std::cout << "Button clicked!\n";
        }
        ImGui::End();

        ImGui::Begin("Video player2");
        for (auto& v : allFmvs)
        {
            if (ImGui::Button(v.c_str()))
            {
                std::cout << "Play " << v.c_str() << std::endl;
            }
        }
        ImGui::End();


        // Rendering
        glClearColor(0.6f, 0.6f, 0.6f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui::Render();

        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
//            std::cout << gluErrorString(error) << std::endl;
        }

        SDL_GL_SwapWindow(window);
    }

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
    return 0;
}
