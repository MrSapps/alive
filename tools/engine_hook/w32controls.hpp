#pragma once

#include <windows.h>
#include <functional>
#include <memory>
#include <vector>
#include <set>

class BaseWindow
{
public:
    HWND Hwnd() const;
    void Show();
    void Destroy();
    virtual ~BaseWindow();
protected:
    HWND mHwnd = NULL;
};
using UP_BaseWindow = std::unique_ptr<BaseWindow>;

class BaseDialog;
class BaseControl : public BaseWindow
{
public:
    BaseControl(BaseDialog* parentDialog, DWORD id);
    ~BaseControl();
    virtual bool HandleMessage(WPARAM wparam, LPARAM lParam) = 0;
protected:
    BaseDialog* mParent = nullptr;
    DWORD mId = 0;
};

class BaseDialog : public BaseWindow
{
public:
    void AddControl(BaseControl* ptr);
    void RemoveControl(BaseControl* ptr);
    bool Create(HINSTANCE instance, LPCSTR dialogId);
protected:
    virtual BOOL Proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    virtual BOOL CreateControls() = 0;
    static BOOL CALLBACK StaticDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
private:
    bool HandleControlMessages(WPARAM wparam, LPARAM lParam);
    std::set<BaseControl*> mControls;
};

class ListBox : public BaseControl
{
public:
    using BaseControl::BaseControl;
    void AddString(const std::string& str);
    void Clear();
    virtual bool HandleMessage(WPARAM wparam, LPARAM lParam) override;
};

class Button : public BaseControl
{
public:
    using BaseControl::BaseControl;
    void OnClicked(std::function<void()> onClick);
    virtual bool HandleMessage(WPARAM wparam, LPARAM lParam) override;
private:
    std::function<void()> mOnClicked;
};

class TextBox : public BaseControl
{
public:
    using BaseControl::BaseControl;
    void OnTextChanged(std::function<void()> onChanged);
    virtual bool HandleMessage(WPARAM wparam, LPARAM lParam) override;
    std::string GetText();
private:
    std::function<void()> mOnChanged;
};
