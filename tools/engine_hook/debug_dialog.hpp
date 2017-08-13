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

struct SoundPriorityData
{
    std::string mName;
    DWORD mHitCount;
};

inline bool operator < (const SoundPriorityData& l, const SoundPriorityData& r)
{
    return l.mName < r.mName;
}

class DebugDialog : public BaseDialog
{
public:
    DebugDialog();
    ~DebugDialog();
    void LogAnimation(const std::string& name);
    void SetActiveVab(const std::string& vab);
    void LogSound(DWORD program, DWORD note);
    void OnReloadAnimJson(std::function<void()> fnOnReload) { mOnReloadJson = fnOnReload; }
protected:
    virtual BOOL CreateControls() override;
    virtual BOOL Proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) override;
private:
    void SyncAnimListBoxData();
    void ClearAnimListBoxAndAnimData();
    void ReloadAnimJson();
    void TriggerRefreshTimer();
    void ClearSoundData();
    void  SyncSoundListBoxData();

    std::unique_ptr<class Button> mResetAnimLogsButton;
    std::unique_ptr<class Button> mClearSoundLogsButton;
    std::unique_ptr<class Button> mUpdateAnimLogsNowButton;
    std::unique_ptr<class TextBox> mAnimFilterTextBox;
    std::unique_ptr<class Button> mReloadAnimJsonButton;
    std::unique_ptr<Timer> mRefreshTimer;

    std::unique_ptr<Label> mActiveVabLabel;

    std::unique_ptr<class ListBox> mAnimListBox;
    std::unique_ptr<class ListBox> mSoundsListBox;

    std::set<AnimPriorityData> mAnims;

    std::string mActiveVab;
    std::set<SoundPriorityData> mSounds;

    std::function<void()> mOnReloadJson;
};
