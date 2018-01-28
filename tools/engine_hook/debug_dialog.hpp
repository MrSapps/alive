#pragma once

#include <windows.h>
#include <memory>
#include <vector>
#include <functional>
#include <set>
#include "w32controls.hpp"
#include "half_float.hpp"

struct BaseAnimatedWithPhysicsGameObject;

struct AnimPriorityData
{
    std::string mName;
    DWORD mLastFrame;
    DWORD mHitCount;
};

inline bool operator < (const AnimPriorityData& l, const AnimPriorityData& r)
{
    return l.mName < r.mName;
}

struct SoundPriorityData
{
    std::string mName;
    DWORD mLastFrame;
    DWORD mHitCount;
};

inline bool operator < (const SoundPriorityData& l, const SoundPriorityData& r)
{
    return l.mName < r.mName;
}

struct DeltaInfo
{
    HalfFloat prevX = 0;
    HalfFloat prevY = 0;
    HalfFloat preVX = 0;
    HalfFloat preVY = 0;
};

struct DeltaRecord
{
    DWORD f;
    f64 x;
    f64 y;
    f64 velx;
    f64 vely;
    std::string ToString() const;
};

struct BaseObj;

class DebugDialog : public BaseDialog
{
public:
    DebugDialog();
    ~DebugDialog();
    void LogAnimation(const std::string& name);
    void SetActiveVab(const std::string& vab);
    void LogSound(DWORD program, DWORD note);
    void OnReloadAnimJson(std::function<void()> fnOnReload) { mOnReloadJson = fnOnReload; }
    void OnFrameEnd();
    void LogMusic(const std::string& seqName);
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

    std::unique_ptr<Button> mResetAnimLogsButton;
    std::unique_ptr<Button> mClearSoundLogsButton;
    std::unique_ptr<Button> mUpdateAnimLogsNowButton;
    std::unique_ptr<TextBox> mAnimFilterTextBox;
    std::unique_ptr<Button> mReloadAnimJsonButton;
    std::unique_ptr<W32Timer> mRefreshTimer;

    std::unique_ptr<Label> mActiveVabLabel;

    std::unique_ptr<Label> mSelectedObjectLabel;
    std::unique_ptr<Button> mClearSelectObjectDeltas;
    std::unique_ptr<ListBox> mObjectDeltasListBox;

    std::unique_ptr<Button> mRefreshObjectListButton;
    std::unique_ptr<ListBox> mObjectsListBox;
    long mSelectedPointer = 0;

    std::unique_ptr<ListBox> mAnimListBox;
    std::unique_ptr<ListBox> mSoundsListBox;

    std::set<AnimPriorityData> mAnims;

    std::string mActiveVab;
    std::set<SoundPriorityData> mSounds;

    std::function<void()> mOnReloadJson;

    DeltaInfo mDeltaInfo;

    std::vector<DeltaRecord> mDeltas;
    void SyncDeltaListBoxData();
private:
    void RecordObjectDeltas(BaseAnimatedWithPhysicsGameObject& selected);
};
