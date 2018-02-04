#pragma once

#include <windows.h>
#include <functional>
#include <memory>
#include <vector>
#include <set>

class BaseWindow
{
public:
    HWND Hwnd() const;
    void Show();
    void Destroy();
    virtual ~BaseWindow();
protected:
    HWND mHwnd = NULL;
};
using UP_BaseWindow = std::unique_ptr<BaseWindow>;

class BaseDialog;
class EventSink : public BaseWindow
{
public:
    EventSink(BaseDialog* parentDialog);
    ~EventSink();
    virtual bool HandleMessage(WPARAM wparam, LPARAM lParam) = 0;
protected:
    BaseDialog* mParent = nullptr;
    DWORD mId = 0;
};

class BaseControl : public EventSink
{
public:
    BaseControl(BaseDialog* parentDialog, DWORD id);
    void SetEnabled(bool enable);
    void SetText(const std::string& str);
protected:
    BaseDialog* mParent = nullptr;
    DWORD mId = 0;
};

class W32Timer;

class BaseDialog : public BaseWindow
{
public:
    void AddControl(EventSink* ptr);
    void RemoveControl(EventSink* ptr);
    bool Create(HINSTANCE instance, LPCSTR dialogId, bool modal = false);
    void RemoveTimer(W32Timer* timer);
    void AddTimer(W32Timer* pTimer);
protected:
    virtual BOOL Proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    virtual BOOL CreateControls() = 0;
    static BOOL CALLBACK StaticDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
private:
    bool HandleControlMessages(WPARAM wparam, LPARAM lParam);
    std::set<W32Timer*> mTimers;
    std::set<EventSink*> mControls;
};

class ListBox : public BaseControl
{
public:
    using BaseControl::BaseControl;
    void AddString(const std::string& str);
    void Clear();
    DWORD SelectedIndex() const;
    void SetSelectedIndex(DWORD index);
    void OnDoubleClick(std::function<void(const std::string&)> fnOnDoubleClick);
    virtual bool HandleMessage(WPARAM wparam, LPARAM lParam) override;
private:
    std::function<void(const std::string&)> mOnDoubleClick;
};

class BaseButton : public BaseControl
{
public:
    using BaseControl::BaseControl;
    void OnClicked(std::function<void()> onClick);
    virtual bool HandleMessage(WPARAM wparam, LPARAM lParam) override;
protected:
    std::function<void()> mOnClicked;
};

class Button : public BaseButton
{
public:
    using BaseButton::BaseButton;
};

class Label : public BaseControl
{
public:
    using BaseControl::BaseControl;
    virtual bool HandleMessage(WPARAM wparam, LPARAM lParam) override;
};

class W32Timer
{
public:
    explicit W32Timer(BaseDialog* parent);
    ~W32Timer();
    void OnTick(std::function<void()> onTick);
    void Stop();
    void Tick();
    UINT_PTR Id() const { return (UINT_PTR)mId; }
    void Start(DWORD intervalMs);
    bool IsRunning() const { return Id() != 0; }
private:
    static DWORD mIdGen;
    DWORD mId = 0;
    BaseDialog* mParent = nullptr;
    std::function<void()> mOnTick;
    UINT_PTR mTimerId = 0;
};

class RadioButton : public BaseButton
{
public:
    using BaseButton::BaseButton;
    bool IsSelected();
    void SetSelected(bool selected);
};

class TextBox : public BaseControl
{
public:
    using BaseControl::BaseControl;
    void OnTextChanged(std::function<void()> onChanged);
    virtual bool HandleMessage(WPARAM wparam, LPARAM lParam) override;
    std::string GetText();
private:
    std::function<void()> mOnChanged;
};

class CheckBox : public BaseControl
{
public:
    using BaseControl::BaseControl;
    void OnCheckChanged(std::function<void(bool)> onChanged);
    virtual bool HandleMessage(WPARAM wparam, LPARAM lParam) override;
    bool GetChecked();
    void SetChecked(bool checked);
private:
    std::function<void(bool)> mOnChanged;
};