#pragma once

#include "types.hpp"

class Addresses
{
public:
    u32 GetWindowHandle() const;
    u32 error_msgbox() const;
    u32 gdi_draw() const;
    u32 ConvertAbeHdcHandle() const;
    u32 ConvertAbeHdcHandle2() const;
    u32 g_lPDSound_dword_BBC344() const;
    u32 dword_575158() const;
    u32 g_snd_time_dword_BBC33C() const;
    u32 dword_BBBD38() const;
    u32 ddCheatOn() const;
    u32 alwaysDrawDebugText() const;
    u32 gPathData() const;
    u32 currentLevelId() const;
    u32 currentPath() const;
    u32 currentCam() const;
    u32 set_first_camera() const;
    u32 sub_418930() const;
    u32 anim_decode() const;
    u32 get_anim_frame() const;
    u32 end_Frame() const;
    u32 ObjectList() const;
    u32 SND_PlayEx() const;
    u32 SND_seq_play_q() const;
    u32 SND_play_snd_internal_q() const;
    u32 SND_Reload() const;
    u32 gSeqData() const;
    u32 g_seq_data_dword_C13400() const;
    u32 Stub_DirectSoundCreate() const;
    u32 gnFrame();
};

Addresses& Addrs();
