#include "debug_dialog.hpp"
#include "resource.h"
#include <string>
#include <math.h>
#include <functional>
#include "string_util.hpp"
#include "anim_logger.hpp"
#include "game_objects.hpp"

DebugDialog::DebugDialog()
{

}

DebugDialog::~DebugDialog()
{

}

BOOL DebugDialog::CreateControls()
{
    mAnimListBox = std::make_unique<ListBox>(this, IDC_ANIMATIONS);
    mSoundsListBox = std::make_unique<ListBox>(this, IDC_SOUNDS);

    mResetAnimLogsButton = std::make_unique<Button>(this, IDC_ANIM_LOG_RESET);
    mResetAnimLogsButton->OnClicked([&]()
    {
        ClearAnimListBoxAndAnimData();
    });

    mClearSoundLogsButton = std::make_unique<Button>(this, IDC_CLEAR_SOUND_LOGS);
    mClearSoundLogsButton->OnClicked([&]()
    {
        ClearSoundData();
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

    mRefreshTimer = std::make_unique<Timer>(this);
    mRefreshTimer->OnTick([&]() 
    {
        SyncAnimListBoxData();
        SyncSoundListBoxData();
        mRefreshTimer->Stop();
    });

    mActiveVabLabel = std::make_unique<Label>(this, IDC_ACTIVE_VAB);

    mSelectedObjectLabel = std::make_unique<Label>(this, IDC_SELECTED_OBJECT);
    mClearSelectObjectDeltas = std::make_unique<Button>(this, IDC_CLEAR_OBJECT_DELTAS);
    mClearSelectObjectDeltas->OnClicked([&]() 
    {
        mObjectDeltasListBox->Clear();
    });

    mObjectDeltasListBox = std::make_unique<ListBox>(this, IDC_OBJECT_DELTAS);
   
    mRefreshObjectListButton = std::make_unique<Button>(this, IDC_REFRESH_OBJECT_LIST);
    mRefreshObjectListButton->OnClicked([&]() 
    {
        mObjectsListBox->Clear();

        GameObjectList::Objs* pObjs = GameObjectList::GetObjectsPtr();
        for (int i = 0; i < pObjs->mCount; i++)
        {
            DWORD ptrValue = reinterpret_cast<DWORD>(pObjs->mPointerToObjects[i]);

            std::string typeString = GameObjectList::AeTypeToString(pObjs->mPointerToObjects[i]->mTypeId);

            std::string name = std::to_string(ptrValue) + "_" + typeString;

            mObjectsListBox->AddString(name);
        }

    });

    mObjectsListBox = std::make_unique<ListBox>(this, IDC_OBJECT_LIST);
    mObjectsListBox->OnDoubleClick([&](const std::string& item) 
    {
        auto ptrValueStr = string_util::split(item, '_')[0];
        mSelectedPointer = std::stol(ptrValueStr);
        mSelectedObjectLabel->SetText(ptrValueStr);
    });

    return TRUE;
}

BOOL DebugDialog::Proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    // Closing this dialog should completely end the game
    if (message == WM_CLOSE)
    {
        ::PostQuitMessage(0);
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

    TriggerRefreshTimer();
}

void DebugDialog::SetActiveVab(const std::string& vab)
{
    mActiveVab = vab;
    mActiveVabLabel->SetText(vab);
}

void DebugDialog::OnFrameEnd()
{
    if (mSelectedPointer)
    {
        GameObjectList::BaseObj* pSelected = reinterpret_cast<GameObjectList::BaseObj*>(mSelectedPointer);
        GameObjectList::Objs* pObjs = GameObjectList::GetObjectsPtr();
        for (int i = 0; i < pObjs->mCount; i++)
        {
            if (pSelected == pObjs->mPointerToObjects[i])
            {
               // LOG_INFO("XPOS: " << pSelected->xpos().AsDouble());
            }
        }
    }
}

void DebugDialog::LogSound(DWORD program, DWORD note)
{
    const std::string name = GetAnimLogger().LookUpSoundEffect(mActiveVab, program, note);

    DWORD hitCount = 0;
    auto it = mSounds.find(SoundPriorityData{ name, 0 });
    if (it != std::end(mSounds))
    {
        hitCount = it->mHitCount;
        mSounds.erase(it);
    }

    SoundPriorityData data{ name, hitCount + 1 };
    mSounds.insert(data);

    TriggerRefreshTimer();
}

void DebugDialog::SyncAnimListBoxData()
{
    const DWORD sel = mAnimListBox->SelectedIndex();

    mAnimListBox->Clear();

    std::string strFilter = mAnimFilterTextBox->GetText();
    for (auto& d : mAnims)
    {
        if (strFilter.empty() || string_util::StringFilter(d.mName.c_str(), strFilter.c_str()))
        {
            mAnimListBox->AddString(d.mName + "(" + std::to_string(d.mHitCount) + ")");
        }
    }

    mAnimListBox->SetSelectedIndex(sel);
}

void DebugDialog::ClearAnimListBoxAndAnimData()
{
    mAnims.clear();
    mAnimListBox->Clear();
}

void DebugDialog::ReloadAnimJson()
{
    if (mOnReloadJson)
    {
        mOnReloadJson();
    }
}

void DebugDialog::TriggerRefreshTimer()
{
    if (!mRefreshTimer->IsRunning())
    {
        mRefreshTimer->Start(500);
    }
}

void DebugDialog::ClearSoundData()
{
    mSounds.clear();
    mSoundsListBox->Clear();
}

void DebugDialog::SyncSoundListBoxData()
{
    const DWORD selected = mSoundsListBox->SelectedIndex();

    mSoundsListBox->Clear();

    for (auto& d : mSounds)
    {
        mSoundsListBox->AddString(d.mName + "(" + std::to_string(d.mHitCount) + ")");
    }
    mSoundsListBox->SetSelectedIndex(selected);
}

void DebugDialog::LogMusic(const std::string& seqName)
{
    DWORD hitCount = 0;
    auto it = mSounds.find(SoundPriorityData{ seqName, 0 });
    if (it != std::end(mSounds))
    {
        hitCount = it->mHitCount;
        mSounds.erase(it);
    }

    SoundPriorityData data{ seqName, hitCount + 1 };
    mSounds.insert(data);

    TriggerRefreshTimer();
}
