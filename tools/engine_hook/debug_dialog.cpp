#include "debug_dialog.hpp"
#include "resource.h"
#include <string>
#include <math.h>
#include <functional>
#include "string_util.hpp"
#include "anim_logger.hpp"
#include "game_functions.hpp"
#include "game_objects.hpp"
#include "demo_hooks.hpp"

extern std::string gStartDemoPath;
extern bool gForceDemo;

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

    mRefreshTimer = std::make_unique<W32Timer>(this);
    mRefreshTimer->OnTick([&]() 
    {
        SyncAnimListBoxData();
        SyncSoundListBoxData();
        SyncDeltaListBoxData();
        mRefreshTimer->Stop();
    });

    mActiveVabLabel = std::make_unique<Label>(this, IDC_ACTIVE_VAB);

    mSelectedObjectLabel = std::make_unique<Label>(this, IDC_SELECTED_OBJECT);
    mClearSelectObjectDeltas = std::make_unique<Button>(this, IDC_CLEAR_OBJECT_DELTAS);
    mClearSelectObjectDeltas->OnClicked([&]() 
    {
        mObjectDeltasListBox->Clear();
        mDeltas.clear();
        mDeltaInfo = {};
    });

    mObjectDeltasListBox = std::make_unique<ListBox>(this, IDC_OBJECT_DELTAS);
   
    mRefreshObjectListButton = std::make_unique<Button>(this, IDC_REFRESH_OBJECT_LIST);
    mRefreshObjectListButton->OnClicked([&]() 
    {
        mObjectsListBox->Clear();

        auto pObjs = GameObjectList::GetObjectsPtr();
        for (int i = 0; i < pObjs->mCount; i++)
        {
            DWORD ptrValue = reinterpret_cast<DWORD>(pObjs->mArray[i]);

            std::string typeString = GameObjectList::AeTypeToString(pObjs->mArray[i]->field_4_typeId);

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
        mDeltaInfo = {};
    });

    mUpdateFakeInput = [&](bool enable)
    {
        auto text = mFakeInputEdit->GetText();
        Demo_SetFakeInputEnabled(enable);
        if (enable)
        {
            Demo_SetFakeInputValue(text);
        }
    };

    mFakeInputEdit = std::make_unique<TextBox>(this, IDC_FAKE_INPUT);
    mFakeInputEdit->OnTextChanged([&]()
    {
        mUpdateFakeInput(mFakeInputCheckBox->GetChecked());
    });

    mFakeInputCheckBox = std::make_unique<CheckBox>(this, IDC_FAKE_INPUT_ENABLE);
    mFakeInputCheckBox->OnCheckChanged([&](bool enable)
    {
        mUpdateFakeInput(enable);
    });

    mDDNoSkip = std::make_unique<CheckBox>(this, IDC_DD_NOSKIP);
    mDDNoSkip->SetChecked(true);
    mDDNoSkip->OnCheckChanged([&](bool enable)
    {
        Vars().gb_ddNoSkip_5CA4D1.Set(enable ? 1 : 0);
    });
    
    mDemoPathTextBox = std::make_unique<TextBox>(this, IDC_DEMO_PATH);
    mDemoPathTextBox->SetText("TEST");

    mPlayDemoButton = std::make_unique<Button>(this, IDC_PLAY_DEMO);
    mPlayDemoButton->OnClicked([&]()
    {
        gStartDemoPath = mDemoPathTextBox->GetText();
        if (!gStartDemoPath.empty())
        {
            gForceDemo = true;
        }
    });
    mPlayDemoButton->SetEnabled(false); // randomly crashes

    mRecordStopButton = std::make_unique<Button>(this, IDC_REC_STOP);
    mRecordStopButton->OnClicked([&]()
    {
        mRecording = !mRecording;
        if (mRecording)
        {
            StartDemoRecording(mDemoPathTextBox->GetText());
            mRecordStopButton->SetText("Stop recording");
        }
        else
        {
            mRecordStopButton->SetText("Start recording");
            EndDemoRecording(mDemoPathTextBox->GetText());
        }
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
    auto it = mAnims.find(AnimPriorityData{ name, 0, 0 });
    if (it != std::end(mAnims))
    {
        hitCount = it->mHitCount;
        mAnims.erase(it);
    }


    AnimPriorityData data{ name, Vars().gnFrame.Get(), hitCount+1 };
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
        auto pSelected = reinterpret_cast<BaseAnimatedWithPhysicsGameObject*>(mSelectedPointer);
        auto pObjs = GameObjectList::GetObjectsPtr();
        for (int i = 0; i < pObjs->mCount; i++)
        {
            if (pSelected == pObjs->mArray[i])
            {
                RecordObjectDeltas(*pSelected);
                return;
            }
        }
    }
}

void DebugDialog::LogSound(DWORD program, DWORD note)
{
    const std::string name = GetAnimLogger().LookUpSoundEffect(mActiveVab, program, note);

    DWORD hitCount = 0;
    auto it = mSounds.find(SoundPriorityData{ name, 0, 0 });
    if (it != std::end(mSounds))
    {
        hitCount = it->mHitCount;
        mSounds.erase(it);
    }

    SoundPriorityData data{ name, Vars().gnFrame.Get(), hitCount + 1 };
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
            mAnimListBox->AddString(d.mName + "(F:" + std::to_string(d.mLastFrame) + " H:" + std::to_string(d.mHitCount) + ")");
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
        mSoundsListBox->AddString(d.mName + "(F:" + std::to_string(d.mLastFrame) + " H:" + std::to_string(d.mHitCount) + ")");
    }
    mSoundsListBox->SetSelectedIndex(selected);
}

void DebugDialog::LogMusic(const std::string& seqName)
{
    DWORD hitCount = 0;
    auto it = mSounds.find(SoundPriorityData{ seqName, 0, 0 });
    if (it != std::end(mSounds))
    {
        hitCount = it->mHitCount;
        mSounds.erase(it);
    }

    SoundPriorityData data{ seqName, Vars().gnFrame.Get(), hitCount + 1 };
    mSounds.insert(data);

    TriggerRefreshTimer();
}

void DebugDialog::SyncDeltaListBoxData()
{
    const DWORD selected = mObjectDeltasListBox->SelectedIndex();

    mObjectDeltasListBox->Clear();

    for (auto& d : mDeltas)
    {
        mObjectDeltasListBox->AddString(d.ToString());
    }
    mObjectDeltasListBox->SetSelectedIndex(selected);
}

void DebugDialog::RecordObjectDeltas(BaseAnimatedWithPhysicsGameObject& obj)
{
    if (mDeltaInfo.prevX != obj.xpos() ||
        mDeltaInfo.prevY != obj.ypos() ||
        mDeltaInfo.preVX != obj.velocity_x() ||
        mDeltaInfo.preVY != obj.velocity_y())
    {
        mDeltas.push_back(DeltaRecord
        {
            Vars().gnFrame.Get(),
            (obj.xpos() - mDeltaInfo.prevX).AsDouble(),
            (obj.ypos() - mDeltaInfo.prevY).AsDouble(),
            (obj.velocity_x() - mDeltaInfo.preVX).AsDouble(),
            (obj.velocity_y() - mDeltaInfo.preVY).AsDouble()
        });
        TriggerRefreshTimer();
    }

    mDeltaInfo.prevX = obj.xpos();
    mDeltaInfo.prevY = obj.ypos();
    mDeltaInfo.preVX = obj.velocity_x();
    mDeltaInfo.preVY = obj.velocity_y();
}

std::string DeltaRecord::ToString() const
{
    return 
        " F: " +
        std::to_string(f) +
        " x:" +
        std::to_string(x) + 
        " y:" +
        std::to_string(y) +
        " velx:" +
        std::to_string(velx) + 
        " vely: " +
        std::to_string(vely);
}
