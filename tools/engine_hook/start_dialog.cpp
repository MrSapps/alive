#include "start_dialog.hpp"
#include "window_hooks.hpp"
#include "resource.h"
#include <string>

BOOL StartDialog::CreateControls()
{
    CenterWnd(mHwnd);

    mStartNormal = std::make_unique<RadioButton>(this, IDC_START_NORMAL);
    mStartNormal->OnClicked([&]()
    {
        mCoutDownTimer->Stop();
        mCountDownLabel->SetText("");
        mStartMode = eNormal;
    });

    mStartFeeco = std::make_unique<RadioButton>(this, IDC_START_FEECO);
    mStartFeeco->OnClicked([&]()
    {
        mCoutDownTimer->Stop();
        mCountDownLabel->SetText("");
        mStartMode = eStartFeeco;
    });

    mStartMenuDirect = std::make_unique<RadioButton>(this, IDC_START_MENU_DIRECT);
    mStartMenuDirect->OnClicked([&]()
    {
        mCoutDownTimer->Stop();
        mCountDownLabel->SetText("");
        mStartMode = eStartMenuDirect;
    });

    mGoButton = std::make_unique<Button>(this, IDC_BUTTON_GO);
    mGoButton->OnClicked([&]()
    {
        EndDialog(mHwnd, 0);
    });

    mCountDownLabel = std::make_unique<Label>(this, IDC_COUNT_DOWN);
    mCoutDownTimer = std::make_unique<Timer>(this, 1000);
    mCoutDownTimer->OnTick([&]() 
    {
        OnCounter();
    });

    // Set default selection
    mStartMenuDirect->SetSelected(true);
    mStartMode = eStartMenuDirect;

    OnCounter();

    return TRUE;
}

void StartDialog::OnCounter()
{
    mCounter--;
    mCountDownLabel->SetText("Auto start in " + std::to_string(mCounter) + " seconds..");
    if (mCounter == 0)
    {
        EndDialog(mHwnd, 0);
    }
}
