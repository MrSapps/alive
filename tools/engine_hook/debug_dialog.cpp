#include "debug_dialog.hpp"
#include "resource.h"

extern HMODULE gDllHandle;

class ListBox : public BaseWindow
{
public:
    explicit ListBox(HWND hwnd)
    {
        mHwnd = hwnd;
    }

    void AddString(const char* str)
    {
        ::SendMessage(mHwnd, LB_ADDSTRING, 0, reinterpret_cast<WPARAM>(str));
    }
};

void BaseWindow::Show()
{
    ::ShowWindow(mHwnd, SW_SHOW);
}

void BaseWindow::Destroy()
{
    ::DestroyWindow(mHwnd);
}

BaseWindow::~BaseWindow()
{

}

BOOL CALLBACK DlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_CLOSE)
    {
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
        PostQuitMessage(0);
        return TRUE;
    }

    DebugDialog* thisPtr = (DebugDialog*)GetWindowLongPtr(hwnd, GWL_USERDATA);
    if (thisPtr)
    {
        return thisPtr->Proc(hwnd, message, wParam, lParam);
    }

    if (message == WM_INITDIALOG)
    {
        SetWindowLongPtr(hwnd, GWL_USERDATA, (LONG_PTR)lParam);
        thisPtr = (DebugDialog*)GetWindowLongPtr(hwnd, GWL_USERDATA);
        return thisPtr->Proc(hwnd, message, wParam, lParam);
    }

    return FALSE;
}

bool DebugDialog::Create(LPCSTR dialogId)
{
    mHwnd = ::CreateDialogParam(gDllHandle, dialogId, NULL, DlgProc, reinterpret_cast<LPARAM>(this));
    return mHwnd != NULL;
}

BOOL DebugDialog::Proc(HWND hwnd, UINT message, WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    switch (message)
    {
    case WM_INITDIALOG:
    {
        HWND hListBox = GetDlgItem(hwnd, IDC_ANIMATIONS);
        auto listbox = std::make_unique<ListBox>(hListBox);
        listbox->AddString("Testing");
        mWindows.push_back(std::move(listbox));
    }
    return FALSE;
    }

    return FALSE;
}
