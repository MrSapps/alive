#include "w32controls.hpp"
#include <vector>
#include <assert.h>

HWND BaseWindow::Hwnd() const
{
    return mHwnd;
}

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

void BaseDialog::AddControl(BaseControl* ptr)
{
    mControls.insert(ptr);
}

void BaseDialog::RemoveControl(BaseControl* ptr)
{
    auto it = mControls.find(ptr);
    if (it != std::end(mControls))
    {
        mControls.erase(it);
    }
}

bool BaseDialog::Create(HINSTANCE instance, LPCSTR dialogId)
{
    mHwnd = ::CreateDialogParam(instance, dialogId, NULL, StaticDlgProc, reinterpret_cast<LPARAM>(this));
    return mHwnd != NULL;
}

BOOL BaseDialog::Proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    mHwnd = hwnd;

    if (message == WM_INITDIALOG)
    {
        return CreateControls();
    }
    else if (message == WM_COMMAND)
    {
        for (auto& control : mControls)
        {
            if (control->HandleMessage(wParam, lParam))
            {
                return TRUE;
            }
        }
    }
    return FALSE;
}

BOOL CALLBACK BaseDialog::StaticDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_CLOSE)
    {
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
        PostQuitMessage(0);
        return TRUE;
    }

    BaseDialog* thisPtr = (BaseDialog*)GetWindowLongPtr(hwnd, GWL_USERDATA);
    if (thisPtr)
    {
        return thisPtr->Proc(hwnd, message, wParam, lParam);
    }

    if (message == WM_INITDIALOG)
    {
        SetWindowLongPtr(hwnd, GWL_USERDATA, (LONG_PTR)lParam);
        thisPtr = (BaseDialog*)GetWindowLongPtr(hwnd, GWL_USERDATA);
        return thisPtr->Proc(hwnd, message, wParam, lParam);
    }

    return FALSE;
}


bool BaseDialog::HandleControlMessages(WPARAM wparam, LPARAM lParam)
{
    for (auto& control : mControls)
    {
        if (control->HandleMessage(wparam, lParam))
        {
            return true;
        }
    }
    return false;
}

BaseControl::BaseControl(BaseDialog* parentDialog, DWORD id)
    : mParent(parentDialog), mId(id)
{
    mHwnd = GetDlgItem(parentDialog->Hwnd(), id);
    assert(mHwnd != NULL);
    mParent->AddControl(this);
}

BaseControl::~BaseControl()
{
    mParent->RemoveControl(this);
}

void ListBox::AddString(const std::string& str)
{
    ::SendMessage(mHwnd, LB_ADDSTRING, 0, reinterpret_cast<WPARAM>(str.c_str()));
}

void ListBox::Clear()
{
    ::SendMessage(mHwnd, LB_RESETCONTENT, 0, 0);
}

bool ListBox::HandleMessage(WPARAM /*wparam*/, LPARAM /*lParam*/)
{
    return FALSE;
}

void Button::OnClicked(std::function<void()> onClick)
{
    mOnClicked = onClick;
}

bool Button::HandleMessage(WPARAM wparam, LPARAM /*lParam*/) 
{
    if (LOWORD(wparam) == mId && mOnClicked)
    {
        mOnClicked();
        return true;
    }
    return false;
}

void TextBox::OnTextChanged(std::function<void()> onChanged)
{
    mOnChanged = onChanged;
}

bool TextBox::HandleMessage(WPARAM wparam, LPARAM /*lParam*/)
{
    if (LOWORD(wparam) == mId)
    {
        switch (HIWORD(wparam))
        {
        case EN_CHANGE:
            if (mOnChanged)
            {
                mOnChanged();
            }
            return true;
        }
        return true;
    }
    return false;
}

std::string TextBox::GetText()
{
    std::vector<char> buffer;
    buffer.resize(GetWindowTextLength(mHwnd));
    GetWindowText(mHwnd, buffer.data(), static_cast<int>(buffer.size()));
    if (buffer.empty())
    {
        return "";
    }
    return std::string(buffer.data(), buffer.size());
}
