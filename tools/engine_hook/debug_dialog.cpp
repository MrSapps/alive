#include "debug_dialog.hpp"

extern HMODULE gDllHandle;

BOOL CALLBACK DlgProc(HWND /*hwnd*/, UINT message, WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    switch (message)
    {
    case WM_INITDIALOG:
        return FALSE;

    default:
        return FALSE;
    }
}

bool DebugDialog::Create(LPCSTR dialogId)
{
    mHwnd = ::CreateDialog(gDllHandle, dialogId, NULL, DlgProc);
    return mHwnd != NULL;
}

void DebugDialog::Show()
{
    ::ShowWindow(mHwnd, SW_SHOW);
}

void DebugDialog::Destroy()
{
    ::DestroyWindow(mHwnd);
}
