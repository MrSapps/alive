#pragma once

#include <windows.h>
#include <memory>
#include <vector>
#include <functional>
#include <set>
#include "w32controls.hpp"

struct AnimPriorityData
{
    std::string mName;
    DWORD mHitCount;
};

inline bool operator < (const AnimPriorityData& l, const AnimPriorityData& r)
{
    return l.mName < r.mName;
}

class DebugDialog : public BaseDialog
{
public:
    DebugDialog();
    ~DebugDialog();
    void LogAnimation(const std::string& name);
    void OnReloadAnimJson(std::function<void()> fnOnReload) { mOnReloadJson = fnOnReload; }
protected:
    virtual BOOL CreateControls() override;
    virtual BOOL Proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) override;
private:
    void SyncAnimListBoxData();
    void ClearAnimListBoxAndAnimData();
    void ReloadAnimJson();

    std::unique_ptr<class Button> mResetAnimLogsButton;
    std::unique_ptr<class Button> mUpdateAnimLogsNowButton;
    std::unique_ptr<class TextBox> mAnimFilterTextBox;
    std::unique_ptr<class Button> mReloadAnimJsonButton;
    std::unique_ptr<Timer> mRefreshTimer;

    std::unique_ptr<class ListBox> mListBox;
    std::set<AnimPriorityData> mAnims;

    std::function<void()> mOnReloadJson;
};
