#include "debug_dialog.hpp"
#include "resource.h"
#include <string>
#include <math.h>
#include <functional>
#include "string_util.hpp"

DebugDialog::DebugDialog()
{
    mTicksSinceLastAnimUpdate = ::GetTickCount();
}

DebugDialog::~DebugDialog()
{

}

BOOL DebugDialog::CreateControls()
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
