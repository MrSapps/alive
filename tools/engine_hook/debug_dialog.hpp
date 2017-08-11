#pragma once

#include <windows.h>
#include <memory>
#include <vector>

class BaseWindow
{
public:
    HWND Hwnd() const { return mHwnd; }
    void Show();
    void Destroy();
    virtual ~BaseWindow();
protected:
    HWND mHwnd = NULL;
};
using UP_BaseWindow = std::unique_ptr<BaseWindow>;

class DebugDialog : public BaseWindow
{
public:
    bool Create(LPCSTR dialogId);
    BOOL Proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
private:
    std::vector<UP_BaseWindow> mWindows;
};
