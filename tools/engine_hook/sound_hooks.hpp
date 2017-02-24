#pragma once

// 0x004EEEC0
char __cdecl SND_CreatePrimaryBuffer(int a1, int a2, int a3);

// 0x004EFD50
int __cdecl SND_DirectSoundRelease();

// 0x004EEAA0
signed int __cdecl SND_Init(unsigned int a1, int a2, int a3);

// 0x004EF680
signed int __cdecl SND_Load(unsigned int a1, const void *aDst, int a3);

// 0x004EEFF0
signed int __cdecl SND_New(int a1, int a2, int aBitRate, unsigned __int8 a4, int a5);

// 0x004EF740
signed int __cdecl SND_PlayEx(unsigned int pSample, unsigned int a2, unsigned int a3, float a4, int a5, signed int a6, int a7);

// 0x004EFA30
signed int __cdecl SND_ReleaseSample_q(int aSound);

// 0x004EF1C0
signed int __cdecl SND_Reload(unsigned int a1, void *a2, const void *a3, unsigned int a4);

// 0x004EF490
signed int __cdecl SND_ReloadFromWriteCursor(unsigned int a1, const void *a2, void *a3);

// 0x004EF350
signed int __fastcall SND_Reload_2(void *thisPtr, void*, int a2, unsigned int a3, unsigned int a4); // __thiscall

// 0x004EEDD0
signed int __cdecl SND_Renew(int aSample);

// 0x004EE990
int __cdecl SND_SetFormat(int a1, unsigned __int8 a2, unsigned __int8 a3);

// 0x004FC360
__int16 __cdecl SND_SetLRVolume(__int16 a1, __int16 a2);

// 0x004EEA00
int __cdecl SND_calc_format(unsigned int* aLPCWAVEFORMATEX, int a2, unsigned __int8 a3, int a4);

// 0x004FDFE0
__int16 __cdecl SND_disable_all_psx_sound_channels();

// 0x004FD900
__int16 __cdecl SND_get_next_helper(unsigned __int16 a1, char a2, __int16 a3);

// 0x004FCB30
int __cdecl SND_play_snd_internal_q(int a1, int a2, signed int vol_left, signed int a4, signed int a5, int vol_right);

// 0x004FD100 
signed int __cdecl SND_seq_play_q(int aSeqIndx);

// 0x004FC840
signed __int16 __cdecl SND_sound_dat_loader(int a1, __int16 index_unknown);

// 0x004FCFF0
int __cdecl SND_turn_notes_off_q(signed int program, signed int tone);

// 0x004C9FE0
signed __int16 __cdecl SND_vh_related(int a1, __int16 a2);

// 0x004FE010
__int16 __cdecl snd_midi_note_off_q(__int16 aKeyIndex);

// 0x004FDEC0
__int16 __cdecl snd_midi_pitch_bend(__int16 a1, __int16 a2);

// 0x004FDB80
int __cdecl snd_midi_set_tempo(__int16 a1, __int16 a2, __int16 a3);

// 0x004FD6C0
int __cdecl snd_midi_skip_length(int a1, int a2);

// 0x004FCE50
unsigned int __cdecl snd_update_time();

// 0x004FC230
char *__cdecl SND_sub_4FC230();

// 0x004FC5B0
char __cdecl SND_sub_4FC5B0(int a1);

// 0x004FD6D0
signed __int16 __cdecl SND_sub_4FD6D0(int a1, __int16 a2);

// 0x004CA350
void __cdecl load_sounds_q(int aSndPtr);
