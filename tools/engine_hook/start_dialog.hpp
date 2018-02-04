#pragma once
#include "w32controls.hpp"

class StartDialog : public BaseDialog
{
public:
    enum StartMode
    {
        eNormal,
        eStartFeeco,
        eStartMenuDirect,
        eStartBootToDemo
    };

    StartMode GetStartMode() const { return mStartMode; }
    const std::string& DemoPath() const { return mDemoPath; }
protected:
    virtual BOOL CreateControls() override;
private:
    void OnCounter();

    std::unique_ptr<RadioButton> mStartNormal;
    std::unique_ptr<RadioButton> mStartFeeco;
    std::unique_ptr<RadioButton> mStartMenuDirect;
    std::unique_ptr<RadioButton> mStartBootToDemo;

    std::unique_ptr<RadioButton> mMusicOn;
    std::unique_ptr<RadioButton> mMusicOff;


    std::unique_ptr<Label> mCountDownLabel;
    std::unique_ptr<W32Timer> mCoutDownTimer;
    std::unique_ptr<Button> mGoButton;

    std::unique_ptr<TextBox> mDemoPathEdit;

    StartMode mStartMode = eNormal;
    std::string mDemoPath;

    DWORD mCounter = 6;
};
