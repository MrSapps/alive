#pragma once

#include <windows.h>

class DebugDialog
{
public:
    bool Create(LPCSTR dialogId);
    void Show();
    HWND Hwnd() const { return mHwnd; }
private:
    HWND mHwnd = NULL;
public:
    void Destroy();
};
