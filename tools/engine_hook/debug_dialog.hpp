#pragma once

#include <windows.h>
#include <memory>
#include <vector>
#include <functional>
#include <set>

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
    BaseControl(DWORD id)
        : mId(id)
    {

    }

    virtual bool HandleMessage(WPARAM wparam, LPARAM lParam) = 0;
protected:
    DWORD mId = 0;
};

class DebugDialog : public BaseWindow
{
public:
    DebugDialog();
    ~DebugDialog();
    bool Create(LPCSTR dialogId);
    BOOL Proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
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

    std::vector<BaseControl*> mControls;

    std::unique_ptr<class ListBox> mListBox;
    std::set<AnimPriorityData> mAnims;
    DWORD mTicksSinceLastAnimUpdate = 0;

    std::function<void()> mOnReloadJson;
};
