#pragma once
#include "w32controls.hpp"

class StartDialog : public BaseDialog
{
public:
    enum StartMode
    {
        eNormal,
        eStartFeeco,
        eStartMenuDirect
    };

    StartMode GetStartMode() const { return mStartMode; }
protected:
    virtual BOOL CreateControls() override;
private:
    void OnCounter();

    std::unique_ptr<RadioButton> mStartNormal;
    std::unique_ptr<RadioButton> mStartFeeco;
    std::unique_ptr<RadioButton> mStartMenuDirect;
    std::unique_ptr<Label> mCountDownLabel;
    std::unique_ptr<Timer> mCoutDownTimer;
    std::unique_ptr<Button> mGoButton;

    StartMode mStartMode = eNormal;
    DWORD mCounter = 3;
};
