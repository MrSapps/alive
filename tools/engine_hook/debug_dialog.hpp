#pragma once

#include <windows.h>
#include <memory>
#include <vector>
#include <functional>
#include <set>

class BaseDialog;

struct AnimPriorityData
{
    std::string mName;
    DWORD mHitCount; 
};

inline bool operator < (const AnimPriorityData& l, const AnimPriorityData& r)
{
    return l.mName < r.mName;
}

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
    void AddControl(BaseControl* ptr)
    {
        mControls.insert(ptr);
    }

    void RemoveControl(BaseControl* ptr)
    {
        auto it = mControls.find(ptr);
        if (it != std::end(mControls))
        {
            mControls.erase(it);
        }
    }

protected:
    virtual BOOL Proc(HWND /*hwnd*/, UINT message, WPARAM wParam, LPARAM lParam)
    {
        if (message == WM_COMMAND)
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

private:
    bool HandleControlMessages(WPARAM wparam, LPARAM lParam)
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

    std::set<BaseControl*> mControls;
};

class DebugDialog : public BaseDialog
{
public:
    DebugDialog();
    ~DebugDialog();
    bool Create(LPCSTR dialogId);
    virtual BOOL Proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) override;
    void LogAnimation(const std::string& name);
    void OnReloadAnimJson(std::function<void()> fnOnReload) { mOnReloadJson = fnOnReload; }
private:
    void SyncAnimListBoxData();
    void ClearAnimListBoxAndAnimData();
    void ReloadAnimJson();

    std::unique_ptr<class Button> mResetAnimLogsButton;
    std::unique_ptr<class Button> mUpdateAnimLogsNowButton;
    std::unique_ptr<class TextBox> mAnimFilterTextBox;
    std::unique_ptr<class Button> mReloadAnimJsonButton;

    std::unique_ptr<class ListBox> mListBox;
    std::set<AnimPriorityData> mAnims;
    DWORD mTicksSinceLastAnimUpdate = 0;

    std::function<void()> mOnReloadJson;
};
