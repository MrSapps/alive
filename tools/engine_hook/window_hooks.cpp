#include "window_hooks.hpp"
#include "debug_dialog.hpp"
#include "resource.h"
#include <memory>
#include <vector>
#include "anim_logger.hpp"

bool gCollisionsEnabled = true;
bool gGridEnabled = false;
static WNDPROC g_pOldProc = 0;
std::unique_ptr<DebugDialog> gDebugUi;
extern HMODULE gDllHandle;

namespace Hooks
{
    Hook<decltype(&::SetWindowLongA)> SetWindowLong(::SetWindowLongA);
}

LRESULT CALLBACK NewWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (!gDebugUi)
    {
        gDebugUi = std::make_unique<DebugDialog>();
        gDebugUi->Create(gDllHandle, MAKEINTRESOURCE(IDD_MAIN));
        gDebugUi->Show();
        CenterWnd(gDebugUi->Hwnd());

        gDebugUi->OnReloadAnimJson([&]() { GetAnimLogger().ReloadJson(); });
    }

    switch (message)
    {
    case WM_CREATE:
        abort();
        break;

    case WM_ERASEBKGND:
    {
        RECT rcWin;
        HDC hDC = GetDC(hwnd);
        GetClipBox((HDC)wParam, &rcWin);
        FillRect(hDC, &rcWin, GetSysColorBrush(COLOR_DESKTOP));  // hBrush can be obtained by calling GetWindowLong()
    }
    return TRUE;

    case WM_GETICON:
    case WM_MOUSEACTIVATE:
    case WM_NCLBUTTONDOWN:
    case WM_NCMOUSELEAVE:
    case WM_KILLFOCUS:
    case WM_SETFOCUS:
    case WM_ACTIVATEAPP:
    case WM_NCHITTEST:
    case WM_ACTIVATE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_LBUTTONDBLCLK:
    case WM_NCCALCSIZE:
    case WM_MOVE:
    case WM_WINDOWPOSCHANGED:
    case WM_WINDOWPOSCHANGING:
    case WM_NCMOUSEMOVE:
    case WM_MOUSEMOVE:
        return DefWindowProc(hwnd, message, wParam, lParam);
    case WM_KEYDOWN:
    {
        if (GetAsyncKeyState('G'))
        {
            gGridEnabled = !gGridEnabled;
        }

        if (GetAsyncKeyState('H'))
        {
            gCollisionsEnabled = !gCollisionsEnabled;
        }

        return g_pOldProc(hwnd, message, wParam, lParam);
    }
    case WM_SETCURSOR:
    {
        // Set the cursor so the resize cursor or whatever doesn't "stick"
        // when we move the mouse over the game window.
        static HCURSOR cur = LoadCursor(0, IDC_ARROW);
        if (cur)
        {
            SetCursor(cur);
        }
        return DefWindowProc(hwnd, message, wParam, lParam);
    }

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        BeginPaint(hwnd, &ps);
        EndPaint(hwnd, &ps);

    }
    return FALSE;
    }

    if (message == WM_DESTROY && gDebugUi)
    {
        gDebugUi->Destroy();
    }

    return g_pOldProc(hwnd, message, wParam, lParam);
}

void CenterWnd(HWND wnd)
{
    RECT r, r1;
    GetWindowRect(wnd, &r);
    GetWindowRect(GetDesktopWindow(), &r1);
    MoveWindow(wnd, ((r1.right - r1.left) - (r.right - r.left)) / 2,
        ((r1.bottom - r1.top) - (r.bottom - r.top)) / 2,
        (r.right - r.left), (r.bottom - r.top), 0);
}

void SubClassWindow()
{
    HWND wnd = FindWindow("ABE_WINCLASS", NULL);
    g_pOldProc = (WNDPROC)SetWindowLong(wnd, GWL_WNDPROC, (LONG)NewWindowProc);

    for (int i = 0; i < 70; ++i)
    {
        ShowCursor(TRUE);
    }

    SetWindowLongA(wnd, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_VISIBLE);

    RECT rc;
    SetRect(&rc, 0, 0, 640, 460);
    AdjustWindowRectEx(&rc, WS_OVERLAPPEDWINDOW | WS_VISIBLE, TRUE, 0);
    SetWindowPos(wnd, NULL, 0, 0, rc.right - rc.left, rc.bottom - rc.top,
        SWP_SHOWWINDOW);


    ShowWindow(wnd, SW_HIDE);

    CenterWnd(wnd);

    ShowWindow(wnd, SW_SHOW);

    InvalidateRect(GetDesktopWindow(), NULL, TRUE);
}

void PatchWindowTitle()
{
    HWND wnd = FindWindow("ABE_WINCLASS", NULL);
    if (wnd)
    {
        const int length = GetWindowTextLength(wnd) + 1;
        std::vector< char > titleBuffer(length + 1);
        if (GetWindowText(wnd, titleBuffer.data(), length))
        {
            std::string titleStr(titleBuffer.data());
            titleStr += " under ALIVE hook";
            SetWindowText(wnd, titleStr.c_str());
        }
    }
}

LONG WINAPI Hook_SetWindowLongA(HWND hWnd, int nIndex, LONG dwNewLong)
{
    if (nIndex == GWL_STYLE)
    {
        dwNewLong = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
    }
    return Hooks::SetWindowLong.Real()(hWnd, nIndex, dwNewLong);
}
