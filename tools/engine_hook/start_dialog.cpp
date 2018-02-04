#include "start_dialog.hpp"
#include "window_hooks.hpp"
#include "resource.h"
#include <string>

extern bool gNoMusic;

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

    mStartBootToDemo = std::make_unique<RadioButton>(this, IDC_START_DEMO);
    mStartMenuDirect->OnClicked([&]()
    {
        mCoutDownTimer->Stop();
        mCountDownLabel->SetText("");
        mStartMode = eStartBootToDemo;
    });

    mDemoPathEdit = std::make_unique<TextBox>(this, IDC_DEMO_PATH);
    mDemoPathEdit->OnTextChanged([&]()
    {
        mDemoPath = mDemoPathEdit->GetText();
    });
    mDemoPath = "ATTR0000.SAV";
    mDemoPathEdit->SetText(mDemoPath);

    mGoButton = std::make_unique<Button>(this, IDC_BUTTON_GO);
    mGoButton->OnClicked([&]()
    {
        EndDialog(mHwnd, 0);
    });

    mCountDownLabel = std::make_unique<Label>(this, IDC_COUNT_DOWN);
    mCoutDownTimer = std::make_unique<W32Timer>(this);
    mCoutDownTimer->OnTick([&]() 
    {
        OnCounter();
    });
    mCoutDownTimer->Start(1000);

    // Set default selection
    mStartBootToDemo->SetSelected(true);
    mStartMode = eStartBootToDemo;

    OnCounter();

    mMusicOn = std::make_unique<RadioButton>(this, IDC_MUSIC_ON);
    mMusicOn->OnClicked([&]()
    {
        gNoMusic = false;
    });

    mMusicOff = std::make_unique<RadioButton>(this, IDC_MUSIC_OFF);
    mMusicOff->OnClicked([&]()
    {
        gNoMusic = true;
    });

    mMusicOff->SetSelected(true);
    gNoMusic = true;

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
