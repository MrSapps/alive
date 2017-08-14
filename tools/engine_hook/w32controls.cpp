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

void BaseDialog::AddControl(EventSink* ptr)
{
    mControls.insert(ptr);
}

void BaseDialog::RemoveControl(EventSink* ptr)
{
    auto it = mControls.find(ptr);
    if (it != std::end(mControls))
    {
        mControls.erase(it);
    }
}

bool BaseDialog::Create(HINSTANCE instance, LPCSTR dialogId, bool modal)
{
    if (modal)
    {
        if (!::DialogBoxParam(instance, dialogId, NULL, StaticDlgProc, reinterpret_cast<LPARAM>(this)))
        {
            return false;
        }
        return true;
    }
    else
    {
        mHwnd = ::CreateDialogParam(instance, dialogId, NULL, StaticDlgProc, reinterpret_cast<LPARAM>(this));
        return mHwnd != NULL;
    }
}

void BaseDialog::RemoveTimer(Timer* timer)
{
    auto it = mTimers.find(timer);
    if (it != std::end(mTimers))
    {
        mTimers.erase(it);
    }
}

void BaseDialog::AddTimer(Timer* pTimer)
{
    mTimers.insert(pTimer);
}

BOOL BaseDialog::Proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    mHwnd = hwnd;

    if (message == WM_INITDIALOG)
    {
        return CreateControls();
    }
    else if (message == WM_TIMER)
    {
        for (auto& timer : mTimers)
        {
            if (timer->Id() == wParam)
            {
                timer->Tick();
            }
        }
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
    BaseDialog* thisPtr = (BaseDialog*)GetWindowLongPtr(hwnd, GWL_USERDATA);
    if (thisPtr)
    {
        if (message == WM_CLOSE)
        {
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
            if (!thisPtr->Proc(hwnd, message, wParam, lParam))
            {
                EndDialog(hwnd, 0);
                return TRUE;
            }
        }
        else
        {
            return thisPtr->Proc(hwnd, message, wParam, lParam);
        }
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

EventSink::EventSink(BaseDialog* parentDialog)
    : mParent(parentDialog)
{
    mParent->AddControl(this);
}

EventSink::~EventSink()
{
    mParent->RemoveControl(this);
}

BaseControl::BaseControl(BaseDialog* parentDialog, DWORD id)
    : EventSink(parentDialog), mId(id)
{
    mHwnd = GetDlgItem(parentDialog->Hwnd(), id);
    assert(mHwnd != nullptr);
}

void ListBox::AddString(const std::string& str)
{
    ::SendMessage(mHwnd, LB_ADDSTRING, 0, reinterpret_cast<WPARAM>(str.c_str()));
}

void ListBox::Clear()
{
    ::SendMessage(mHwnd, LB_RESETCONTENT, 0, 0);
}

DWORD ListBox::SelectedIndex() const
{
    auto ret = ::SendMessage(mHwnd, LB_GETCURSEL, 0, 0);
    if (ret == LB_ERR)
    {
        return 0;
    }
    return ret;
}

void ListBox::SetSelectedIndex(DWORD index)
{
    ::SendMessage(mHwnd, LB_SETCURSEL, index, 0);
}

bool ListBox::HandleMessage(WPARAM wparam, LPARAM /*lParam*/)
{
    if (LOWORD(wparam) == mId)
    {
        if (HIWORD(wparam) == LBN_DBLCLK)
        {
            if (mOnDoubleClick)
            {
                DWORD idx = SelectedIndex();
                DWORD len = ::SendMessage(mHwnd, LB_GETTEXTLEN, idx, 0);
                std::vector<char> buffer(len + 1);
                ::SendMessage(mHwnd, LB_GETTEXT, idx, (LPARAM)buffer.data());
                mOnDoubleClick(buffer.data());
                return true;
            }
        }
    }
    return false;
}

void ListBox::OnDoubleClick(std::function<void(const std::string&)> fnOnDoubleClick)
{
    mOnDoubleClick = fnOnDoubleClick;
}

void BaseButton::OnClicked(std::function<void()> onClick)
{
    mOnClicked = onClick;
}

bool BaseButton::HandleMessage(WPARAM wparam, LPARAM /*lParam*/)
{
    if (LOWORD(wparam) == mId && mOnClicked)
    {
        mOnClicked();
        return true;
    }
    return false;
}

void Label::SetText(const std::string& text)
{
    ::SetWindowText(mHwnd, text.c_str());
}

bool Label::HandleMessage(WPARAM /*wparam*/, LPARAM /*lParam*/)
{
    return false;
}

DWORD Timer::mIdGen = 1;

Timer::Timer(BaseDialog* parent)
    : mParent(parent)
{
    mParent->AddTimer(this);
}

Timer::~Timer()
{
    mParent->RemoveTimer(this);
    Stop();
}

void Timer::Stop()
{
    mTimerId = 0;
    mId = 0;
    ::KillTimer(mParent->Hwnd(), mTimerId);
}

void Timer::Tick()
{
    if (mOnTick)
    {
        mOnTick();
    }
}

void Timer::Start(DWORD intervalMs)
{
    assert(mTimerId == 0);
    mId = mIdGen;
    mIdGen++;
    mTimerId = ::SetTimer(mParent->Hwnd(), mId, intervalMs, 0);
}

void Timer::OnTick(std::function<void()> onTick)
{
    mOnTick = onTick;
}

bool RadioButton::IsSelected()
{
    return IsDlgButtonChecked(mHwnd, mId) == BST_CHECKED;
}

void RadioButton::SetSelected(bool selected)
{
    ::SendMessage(mHwnd, BM_SETCHECK, selected ? BST_CHECKED : BST_UNCHECKED, 0);
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
