#include <windows.h>
#include <vector>
#include "ddraw7proxy.hpp"

/*static*/ DirectSurface7Proxy* DirectSurface7Proxy::g_Primary;
/*static*/ DirectSurface7Proxy* DirectSurface7Proxy::g_BackBuffer;
/*static*/ DirectSurface7Proxy* DirectSurface7Proxy::g_FakePrimary;

HINSTANCE gDllInstance = NULL;
HRESULT WINAPI NewDirectDrawCreate(GUID* lpGUID, IDirectDraw** lplpDD, IUnknown* pUnkOuter);
using TDirectDrawCreate = decltype(&NewDirectDrawCreate);

inline void FatalExit(const char* msg)
{
    ::MessageBox(NULL, msg, "Error", MB_OK | MB_ICONEXCLAMATION);
}

static HMODULE LoadRealDDrawDll()
{
    char infoBuf[MAX_PATH] = {};
    ::GetSystemDirectory(infoBuf, MAX_PATH);

    strcat_s(infoBuf, "\\ddraw.dll");

    const HMODULE hRealDll = ::LoadLibrary(infoBuf);
    if (!hRealDll)
    {
        FatalExit("Can't load or find real DDraw.dll");
    }
    return hRealDll;
}

template<class T>
inline T TGetProcAddress(HMODULE hDll, const char* func)
{
    #pragma warning(suppress: 4191)
    return reinterpret_cast<T>(::GetProcAddress(hDll, func));
}

static TDirectDrawCreate GetFunctionPointersToRealDDrawFunctions(HMODULE hRealDll)
{
    auto ptr = TGetProcAddress<TDirectDrawCreate>(hRealDll, "DirectDrawCreate");
    if (!ptr)
    {
        FatalExit("Can't find DirectDrawCreate function in real dll");
    }
    return ptr;
}


// =================
WNDPROC g_pOldProc = 0;

static LRESULT CALLBACK NewWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
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

    return g_pOldProc(hwnd, message, wParam, lParam);
}

static void CenterWnd(HWND wnd)
{
    RECT r, r1;
    GetWindowRect(wnd, &r);
    GetWindowRect(GetDesktopWindow(), &r1);
    MoveWindow(wnd, ((r1.right - r1.left) - (r.right - r.left)) / 2,
        ((r1.bottom - r1.top) - (r.bottom - r.top)) / 2,
        (r.right - r.left), (r.bottom - r.top), 0);
}

static void SubClassWindow()
{
    HWND wnd = FindWindow("ABE_WINCLASS", NULL);
    g_pOldProc = (WNDPROC)SetWindowLong(wnd, GWLP_WNDPROC, (LONG)NewWindowProc);

    for (int i = 0; i < 70; ++i)
    {
        ShowCursor(TRUE);
    }

    RECT rc;
    SetRect(&rc, 0, 0, 640, 480);
    AdjustWindowRectEx(&rc, WS_OVERLAPPEDWINDOW | WS_VISIBLE, TRUE, 0);
    SetWindowPos(wnd, NULL, 0, 0, rc.right - rc.left, rc.bottom - rc.top,
        SWP_SHOWWINDOW);


    ShowWindow(wnd, SW_HIDE);

    CenterWnd(wnd);

    ShowWindow(wnd, SW_SHOW);

    InvalidateRect(GetDesktopWindow(), NULL, TRUE);

    // TODO: Hook this function, setting the style here fails
    SetWindowLong(wnd, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_VISIBLE);
}

static void PatchWindowTitle()
{
    HWND wnd = FindWindow("ABE_WINCLASS", NULL);
    if (wnd)
    {
        const int length = GetWindowTextLength(wnd) + 1;
        std::vector< char > titleBuffer(length+1);
        if (GetWindowText(wnd, titleBuffer.data(), length))
        {
            std::string titleStr(titleBuffer.data());
            titleStr += " under ALIVE hook";
            SetWindowText(wnd, titleStr.c_str());
        }
    }
}

void HookMain()
{
    SubClassWindow();
    PatchWindowTitle();
}

// Proxy DLL entry point
HRESULT WINAPI NewDirectDrawCreate(GUID* lpGUID, IDirectDraw** lplpDD, IUnknown* pUnkOuter)
{
    const HMODULE hDDrawDll = LoadRealDDrawDll();
    const TDirectDrawCreate pRealDirectDrawCreate = GetFunctionPointersToRealDDrawFunctions(hDDrawDll);
    const HRESULT ret = pRealDirectDrawCreate(lpGUID, lplpDD, pUnkOuter);
    if (SUCCEEDED(ret))
    {
        *lplpDD = new DirectDraw7Proxy(*lplpDD);
        HookMain();
    }
    return ret;
}

extern "C" BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID /*lpReserved*/)
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    {
        gDllInstance = hModule;
    }
    return TRUE;
}
