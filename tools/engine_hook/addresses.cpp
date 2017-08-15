#include "addresses.hpp"
#include "game_functions.hpp"
#include "hook_utils.hpp"

Addresses& Addrs()
{
    static Addresses a;
    return a;
}

u32 Addresses::GetWindowHandle() const
{
    return Utils::IsAe() ? 0x004F2C70 : 0x0048E930;
}

u32 Addresses::error_msgbox() const
{
    return Utils::IsAe() ? 0x004F2920 : 0x0048DDC0;
}

u32 Addresses::sub_4EF970() const
{
    // Called by SND_PlayEx - appears to not exist in AO
    return Utils::IsAe() ? 0x004EF970 : 0x0;
}

u32 Addresses::gdi_draw() const
{
    // In AO ConvertAbeHdcHandle and ConvertAbeHdcHandle2 functions are in-lined into this one
    return Utils::IsAe() ? 0x004F21F0 : 0x0048FAF0;
}

u32 Addresses::ConvertAbeHdcHandle() const
{
    return Utils::IsAe() ? 0x4F2150 : 0x0048FA50;
}

u32 Addresses::ConvertAbeHdcHandle2() const
{
    return Utils::IsAe() ? 0x4F21A0 : 0x0048FAA0;
}

u32 Addresses::g_lPDSound_dword_BBC344() const
{
    return Utils::IsAe() ? 0x00BBC344 : 0x00A08264;
}

u32 Addresses::dword_575158() const
{
    return Utils::IsAe() ? 0x575158 : 0x0;
}

u32 Addresses::g_snd_time_dword_BBC33C() const
{
    return Utils::IsAe() ? 0xBBC33C : 0x0;
}

u32 Addresses::dword_BBBD38() const
{
    return Utils::IsAe() ? 0xBBBD38 : 0x0;
}

u32 Addresses::ddCheatOn() const
{
    return Utils::IsAe() ? 0x005CA4B5 : 0x508BF8;
}

u32 Addresses::alwaysDrawDebugText() const
{
    return Utils::IsAe() ? 0x005BC000 : 0x4FF860;
}

u32 Addresses::gPathData() const
{
    return Utils::IsAe() ? 0x00559660 : 0x0;
}

u32 Addresses::currentLevelId() const
{
    return Utils::IsAe() ? 0x5C3030 : 0x0;
}

u32 Addresses::currentPath() const
{
    return Utils::IsAe() ? 0x5C3032 : 0x0;
}

u32 Addresses::currentCam() const
{
    return Utils::IsAe() ? 0x5C3034 : 0x0;
}

u32 Addresses::set_first_camera() const
{
    return Utils::IsAe() ? 0x00401415 : 0x0;
}

u32 Addresses::sub_418930() const
{
    return Utils::IsAe() ? 0x00418930 : 0x0;
}

u32 Addresses::anim_decode() const
{
    return Utils::IsAe() ? 0x0040AC90 : 0x0;
}

u32 Addresses::get_anim_frame() const
{
    return Utils::IsAe() ? 0x0040B730 : 0x0;
}

u32 Addresses::end_Frame() const
{
    return Utils::IsAe() ? 0x004950F0 : 0x0;
}

u32 Addresses::AbeSnap_sub_449930() const
{
    return Utils::IsAe() ? 0x00449930 : 0x0;
}

u32 Addresses::ObjectList() const
{
    return Utils::IsAe() ? 0x00BB47C4 : 0x0;
}

u32 Addresses::SND_PlayEx() const
{
    return Utils::IsAe() ? 0x004EF740 : 0x00493040;
}

u32 Addresses::SND_seq_play_q() const
{
    return Utils::IsAe() ? 0x004FD100 : 0x49DD30;
}

u32 Addresses::SND_play_snd_internal_q() const
{
    return Utils::IsAe() ? 0x004FCB30 : 0x49D730;
}

u32 Addresses::SND_Reload() const
{
    return Utils::IsAe() ? 0x004EF1C0 : 0x00492B20;
}

u32 Addresses::gSeqData() const
{
    return Utils::IsAe() ? 0x00558D50 : 0x4C9E70;
}

u32 Addresses::g_seq_data_dword_C13400() const
{
    return Utils::IsAe() ? 0xC13400 : 0xABFB40;
}

u32 Addresses::Stub_DirectSoundCreate() const
{
    return Utils::IsAe() ? 0x0052C762 : 0x004B6B02;
}
