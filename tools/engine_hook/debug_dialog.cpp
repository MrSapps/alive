#include "debug_dialog.hpp"
#include "resource.h"
#include <string>
#include <math.h>
#include <functional>
#include "string_util.hpp"

extern HMODULE gDllHandle;

class ListBox : public BaseControl
{
public:
    using BaseControl::BaseControl;

    void AddString(const std::string& str)
    {
        ::SendMessage(mHwnd, LB_ADDSTRING, 0, reinterpret_cast<WPARAM>(str.c_str()));
    }

    void Clear()
    {
        ::SendMessage(mHwnd, LB_RESETCONTENT, 0, 0);
    }

    virtual bool HandleMessage(WPARAM /*wparam*/, LPARAM /*lParam*/) override
    {
        return FALSE;
    }
};

class Button : public BaseControl
{
public:
    using BaseControl::BaseControl;

    void OnClicked(std::function<void()> onClick)
    {
        mOnClicked = onClick;
    }

    virtual bool HandleMessage(WPARAM wparam, LPARAM /*lParam*/) override
    {
        if (LOWORD(wparam) == mId && mOnClicked)
        {
            mOnClicked();
            return true;
        }
        return false;
    }

private:
    std::function<void()> mOnClicked;
};

class TextBox : public BaseControl
{
public:
    using BaseControl::BaseControl;

    void OnTextChanged(std::function<void()> onChanged)
    {
        mOnChanged = onChanged;
    }

    virtual bool HandleMessage(WPARAM wparam, LPARAM /*lParam*/) override
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

    std::string GetText()
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

private:
    std::function<void()> mOnChanged;
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

static BOOL CALLBACK DlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
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

DebugDialog::DebugDialog()
{
    mTicksSinceLastAnimUpdate = ::GetTickCount();
}

DebugDialog::~DebugDialog()
{

}

bool DebugDialog::Create(LPCSTR dialogId)
{
    mHwnd = ::CreateDialogParam(gDllHandle, dialogId, NULL, DlgProc, reinterpret_cast<LPARAM>(this));
    return mHwnd != NULL;
}

BOOL DebugDialog::Proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    mHwnd = hwnd;

    switch (message)
    {
        case WM_INITDIALOG:
        {
            mListBox = std::make_unique<ListBox>(this, IDC_ANIMATIONS);

            mResetAnimLogsButton = std::make_unique<Button>(this, IDC_ANIM_LOG_RESET);
            mResetAnimLogsButton->OnClicked([&]()
            {
                ClearAnimListBoxAndAnimData();
            });

            mUpdateAnimLogsNowButton = std::make_unique<Button>(this, IDC_ANIM_LIST_UPDATE_NOW);
            mUpdateAnimLogsNowButton->OnClicked([&]()
            {
                SyncAnimListBoxData();
            });

            mAnimFilterTextBox = std::make_unique<TextBox>(this, IDC_ANIM_FILTER);
            mAnimFilterTextBox->OnTextChanged([&]()
            {
                SyncAnimListBoxData();
            });

            mReloadAnimJsonButton = std::make_unique<Button>(this, IDC_ANIM_JSON_RELOAD);
            mReloadAnimJsonButton->OnClicked([&]()
            {
                ReloadAnimJson();
            });
        }
        return FALSE;
    }

    return BaseDialog::Proc(hwnd, message, wParam, lParam);
}

void DebugDialog::LogAnimation(const std::string& name)
{
    DWORD hitCount = 0;
    auto it = mAnims.find(AnimPriorityData{ name, 0 });
    if (it != std::end(mAnims))
    {
        hitCount = it->mHitCount;
        mAnims.erase(it);
    }

    AnimPriorityData data{ name, hitCount+1 };
    mAnims.insert(data);

    const DWORD tickCount = ::GetTickCount();
    if (abs(static_cast<long>(mTicksSinceLastAnimUpdate - tickCount)) > 500)
    {
        SyncAnimListBoxData();
        mTicksSinceLastAnimUpdate = ::GetTickCount();
    }
}

void DebugDialog::SyncAnimListBoxData()
{
    mListBox->Clear();

    std::string strFilter = mAnimFilterTextBox->GetText();
    for (auto& d : mAnims)
    {
        if (strFilter.empty() || string_util::StringFilter(d.mName.c_str(), strFilter.c_str()))
        {
            mListBox->AddString(d.mName + "(" + std::to_string(d.mHitCount) + ")");
        }
    }
}

void DebugDialog::ClearAnimListBoxAndAnimData()
{
    mAnims.clear();
    mListBox->Clear();
}

void DebugDialog::ReloadAnimJson()
{
    if (mOnReloadJson)
    {
        mOnReloadJson();
    }
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
