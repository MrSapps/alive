#include <windows.h>
#include <vector>
#include "ddraw7proxy.hpp"
#include "detours.h"
#include "logger.hpp"

/*static*/ DirectSurface7Proxy* DirectSurface7Proxy::g_Primary;
/*static*/ DirectSurface7Proxy* DirectSurface7Proxy::g_BackBuffer;
/*static*/ DirectSurface7Proxy* DirectSurface7Proxy::g_FakePrimary;

HINSTANCE gDllInstance = NULL;
HRESULT WINAPI NewDirectDrawCreate(GUID* lpGUID, IDirectDraw** lplpDD, IUnknown* pUnkOuter);
using TDirectDrawCreate = decltype(&NewDirectDrawCreate);

inline void FatalExit(const char* msg)
{
    ::MessageBox(NULL, msg, "Error", MB_OK | MB_ICONEXCLAMATION);
    exit(-1);
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
    g_pOldProc = (WNDPROC)SetWindowLong(wnd, GWL_WNDPROC, (LONG)NewWindowProc);

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

template<class FunctionType, class RealFunctionType = FunctionType>
class Hook
{
public:
    Hook(const Hook&) = delete;
    Hook& operator = (const Hook&) = delete;
    explicit Hook(FunctionType oldFunc)
        : mOldPtr(oldFunc)
    {

    }

    explicit Hook(DWORD oldFunc)
        : mOldPtr(reinterpret_cast<FunctionType>(oldFunc))
    {

    }

    void Install(FunctionType newFunc)
    {
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourAttach(&(PVOID&)mOldPtr, newFunc);
        if (DetourTransactionCommit() != NO_ERROR)
        {
            FatalExit("detouring failed");
        }
    }

    RealFunctionType Real() const
    {
        #pragma warning(suppress: 4191)
        return reinterpret_cast<RealFunctionType>(mOldPtr);
    }

private:
    FunctionType mOldPtr;
};

static int __fastcall set_first_camera_hook(void *thisPtr, void*, __int16 a2, __int16 a3, __int16 a4, __int16 a5, __int16 a6, __int16 a7);
typedef int(__thiscall* set_first_camera_thiscall)(void *thisPtr, __int16 a2, __int16 a3, __int16 a4, __int16 a5, __int16 a6, __int16 a7);

struct anim_struct
{
    void* mVtbl;
    BYTE field_4; // max w?
    BYTE field_5; // max h?
    WORD flags;
    DWORD field_8;
    WORD field_C;
    WORD field_E;
    DWORD field_10;
    DWORD field_14;
    DWORD mFrameTableOffset; // offset to frame table from anim data header
    DWORD field_1C;
    void** field_20; // pointer to a pointer which points to anim data?
    DWORD iDbufPtr;
    DWORD iAnimSize;
    DWORD field_2C;
    DWORD field_30;
    DWORD field_34;
    DWORD field_38;
    DWORD field_3C;
    DWORD field_40;
    DWORD field_44;
    DWORD field_48;
    DWORD field_4C;
    DWORD field_50;
    DWORD field_54;
    DWORD field_58;
    DWORD field_5C;
    DWORD field_60;
    DWORD field_64;
    DWORD field_68;
    DWORD field_6C;
    DWORD field_70;
    DWORD field_74;
    DWORD field_78;
    DWORD field_7C;
    DWORD field_80;
    WORD iRect;
    WORD field_86;
    WORD field_88;
    WORD field_8A;
    DWORD field_8C;
    WORD field_90;
    WORD field_92;
};

void __fastcall anim_decode_hook(anim_struct* thisPtr, void*);
typedef void (__thiscall* anim_decode_thiscall)(anim_struct* thisPtr);

namespace Hooks
{
    Hook<decltype(&::SetWindowLongA)> SetWindowLong(::SetWindowLongA);
    Hook<decltype(&::set_first_camera_hook), set_first_camera_thiscall> set_first_camera(0x00401415);
    Hook<decltype(&::anim_decode_hook), anim_decode_thiscall> anim_decode(0x0040AC90);
}

LONG WINAPI Hook_SetWindowLongA(HWND hWnd, int nIndex, LONG dwNewLong)
{
    if (nIndex == GWL_STYLE)
    {
        dwNewLong = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
    }
    return Hooks::SetWindowLong.Real()(hWnd, nIndex, dwNewLong);
}

static int __fastcall set_first_camera_hook(void *thisPtr, void* , __int16 levelNumber, __int16 pathNumber, __int16 cameraNumber, __int16 screenChangeEffect, __int16 a6, __int16 a7)
{
    // AE Cheat screen, I think we have to fake cheat input or set a bool for this to work :(
    // cameraNumber = 31;

    // Setting to Feco lets us go directly in game, with the side effect that pausing will crash
    // and some other nasties, still its good enough for debugging animations
    levelNumber = 5;

    // Abe "hello" screen when levelNumber is left as the intro level
    cameraNumber = 1;

    // pathNumber = 4;

    // 5 = "flash" on
    // 4 = top to bottom
    // 8 = door effect/sound
    screenChangeEffect = 5;

    return Hooks::set_first_camera.Real()(thisPtr, levelNumber, pathNumber, cameraNumber, screenChangeEffect, a6, a7);
}

void __fastcall anim_decode_hook(anim_struct* thisPtr, void*)
{
    // TODO Hook int __fastcall get_anim_frame_q(int a1, int a2, __int16 a3)  @ 0x004042CD
    // seems its return value can be used to get the anim data frame pointer

    OutputDebugString("anim_decode\n");
    Hooks::anim_decode.Real()(thisPtr);
}

void HookMain()
{
    Hooks::SetWindowLong.Install(Hook_SetWindowLongA);
    Hooks::set_first_camera.Install(set_first_camera_hook);
    Hooks::anim_decode.Install(anim_decode_hook);

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
