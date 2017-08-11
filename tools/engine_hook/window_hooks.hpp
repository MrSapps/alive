#pragma once

#include "hook.hpp"

extern bool gCollisionsEnabled;
extern bool gGridEnabled;

namespace Hooks
{
    extern Hook<decltype(&::SetWindowLongA)> SetWindowLong;
}

LRESULT CALLBACK NewWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void CenterWnd(HWND wnd);
void SubClassWindow();
void PatchWindowTitle();
LONG WINAPI Hook_SetWindowLongA(HWND hWnd, int nIndex, LONG dwNewLong);
