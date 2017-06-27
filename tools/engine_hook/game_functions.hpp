#pragma once

#include <dsound.h>

static IDirectSound8** g_lPDSound_dword_BBC344 = reinterpret_cast<IDirectSound8**>(0x00BBC344);
static DWORD& dword_575158 = *reinterpret_cast<DWORD*>(0x575158);
static DWORD& g_snd_time_dword_BBC33C = *reinterpret_cast<DWORD*>(0xBBC33C);
static DWORD* dword_BBBD38 = reinterpret_cast<DWORD*>(0xBBBD38);

HWND __cdecl GetWindowHandle();
int __cdecl error_msgbox(char* sourceFile, int errorCode, int notUsed, const char* message);
IDirectSoundBuffer *__cdecl sub_4EF970(int soundIndex, int a2);
