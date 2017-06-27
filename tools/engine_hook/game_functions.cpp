#include "game_functions.hpp"

static decltype(&GetWindowHandle) gGetWindowHandle = reinterpret_cast<decltype(&GetWindowHandle)>(0x004F2C70);
HWND __cdecl GetWindowHandle()
{
    return gGetWindowHandle();
}

static decltype(&error_msgbox) gerror_msgbox = reinterpret_cast<decltype(&error_msgbox)>(0x004F2920);
int __cdecl error_msgbox(char* sourceFile, int errorCode, int notUsed, const char* message)
{
    return gerror_msgbox(sourceFile, errorCode, notUsed, message);
}

static decltype(&sub_4EF970) gsub_4EF970 = reinterpret_cast<decltype(&sub_4EF970)>(0x004EF970);
IDirectSoundBuffer *__cdecl sub_4EF970(int soundIndex, int a2)
{
    return gsub_4EF970(soundIndex, a2);
}
