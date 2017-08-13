#include "sound_hooks.hpp"
#include "game_functions.hpp"
#include <detours.h>
#include "logger.hpp"
#include "types.hpp"
#include <assert.h>
#include <map>
#include "seq_name_algorithm.hpp"

#pragma comment(lib, "Winmm.lib")

static Hook<decltype(&::SND_PlayEx)> SND_PlayEx_hook(0x004EF740);
static Hook<decltype(&::SND_seq_play_q)> SND_seq_play_q_hook(0x004FD100);
static Hook<decltype(&::SND_play_snd_internal_q)>  SND_play_snd_internal_q_hook(0x004FCB30);


static void SoundTest();

void InstallSoundHooks()
{
    //SND_PlayEx_hook.Install(SND_PlayEx);
    SND_seq_play_q_hook.Install(SND_seq_play_q);
    SND_play_snd_internal_q_hook.Install(SND_play_snd_internal_q);

    SoundTest();
}

struct SData
{
    union
    {
        DWORD data; 
        struct
        {
            char b1;
            char b2;
            char b3;
            char b4;
        };
    };
};

int __cdecl SND_play_snd_internal_q(int a1, int programNumber, signed int noteAndOtherData, signed int a4, signed int a5, int vol_right)
{
    SData c;
    c.data = noteAndOtherData;

    std::cout << "PLAYING: Program: " << (DWORD)programNumber << " NOTE: " << (DWORD)c.b2 << std::endl;

    return SND_play_snd_internal_q_hook.Real()(a1, programNumber, c.data, a4, a5, vol_right);
}

static decltype(&SND_Reload) gSND_Reload = reinterpret_cast<decltype(&SND_Reload)>(0x004EF1C0);
signed int __cdecl SND_Reload(AliveSoundBuffer* a1, void *a2, const void *a3, unsigned int a4)
{
    return gSND_Reload(a1, a2, a3, a4);
}

signed int __cdecl SND_PlayEx(AliveSoundBuffer* pSound, unsigned int volL, unsigned int volR, float unknown1, IDirectSoundBuffer* pDuplicated, DWORD playFlags, int unknown2)
{
    if (!Vars().g_lPDSound_dword_BBC344.Get())
    {
        return -1;
    }

    if (!pSound)
    {
        Funcs().error_msgbox("C:\\abe2\\code\\POS\\SND.C", 845, -1, "SND_PlayEx: NULL SAMPLE !!!");
        return -1;
    }

    auto pDirectSoundBuffer = pSound->mDirectSoundBuffer;
    if (!pDirectSoundBuffer)
    {
        return -1;
    }

    auto currentTime = timeGetTime();
    Vars().g_snd_time_dword_BBC33C.Set(currentTime);

    auto maxVol = volR;
    if (volL > volR)
    {
        maxVol = volL;
    }

    auto volumeRelated = (signed int)(120 * maxVol * Vars().dword_575158.Get()) >> 14;
    if (volumeRelated >= 0)
    {
        if (volumeRelated > 127)
        {
            volumeRelated = 127;
        }
    }
    else
    {
        volumeRelated = 0;
    }

    if (playFlags & 1)
    {
        playFlags = DSBPLAY_LOOPING;
    }

    if (pSound->mNumChannels & 2)
    {
        DWORD dwStatus = 0;
        if (pDirectSoundBuffer->GetStatus(&dwStatus))
        {
            return -1;
        }

        if (pSound->mIndex & 1)
        {
            auto freq = (signed __int64)((long double)pSound->mSampleRate * unknown1 + 0.5);
            pDirectSoundBuffer->SetFrequency((DWORD)freq);
            pDirectSoundBuffer->SetVolume(Vars().dword_BBBD38.Get()[volumeRelated]);
            pDirectSoundBuffer->SetCurrentPosition(0);
            return 0;
        }
    }
    else
    {
        IDirectSoundBuffer* pDuplicate = Funcs().sub_4EF970(pSound->mIndex, volumeRelated + unknown2);
        if (!pDuplicate)
        {
            return -1;
        }

        HRESULT errorCode = Vars().g_lPDSound_dword_BBC344.Get()->DuplicateSoundBuffer(
            pDirectSoundBuffer,
            &pDuplicate);

        if (errorCode)
        {
            auto errorString = "fix me";// GetDSoundError(errorCode);
            Funcs().error_msgbox("C:\\abe2\\code\\POS\\SND.C", 921, -1, errorString);
            return -1;
        }

        pDirectSoundBuffer = pDuplicate;
        if (pDuplicated)
        {
            pDuplicated = pDuplicate;
        }

    }

 
    auto freq = (signed __int64)((long double)pSound->mSampleRate * unknown1 + 0.5);
    if (freq >= 100)
    {
        if (freq > 100000)
        {
            freq = 100000;
        }
    }
    else
    {
        freq = 100;
    }
    pDirectSoundBuffer->SetFrequency((DWORD)freq);
    

    pDirectSoundBuffer->SetVolume(Vars().dword_BBBD38.Get()[volumeRelated]);
  
    auto panValue = (signed int)(10000 * (volL - volR)
        +((unsigned __int64)(18446744071578977289i64 * (signed int)(10000 * (volL - volR))) >> 32)) >> 6;

    pDirectSoundBuffer->SetPan(((unsigned int)panValue >> 31) + panValue);

    auto ret = pDirectSoundBuffer->Play(0, 0, playFlags) != DSERR_BUFFERLOST;
    if (ret 
        || !SND_Reload(pSound, 0, pSound->mAllocatedBuffer, pSound->mBufferBytes1 / pSound->mBlockAlign)
        || !pDirectSoundBuffer->Play(0, 0, playFlags))
    {
        return 0;
    }

    return -1;
}

struct SeqData
{
    const char* mSeqName;   // The original sound file name! For example MINESAMB.SEQ
    u32 mGeneratedResId;    // This is the SEQ ID generated via ResourceNameHash(mSeqName)
    u16 mIndexUnknown;      // Seems to get used as an index into something else
    u16 mUnknown;
    u32 mSeqIdOrZero;       // Probably a pointer?
};

struct SeqDataTable
{
    SeqData mData[145];
};

SeqDataTable* gSeqData = reinterpret_cast<SeqDataTable*>(0x00558D50);


std::map<u32, const char*> gSeqIdToName;

static void SoundTest()
{

    for (SeqData& data : gSeqData->mData)
    {
        if (data.mSeqName)
        {
            data.mGeneratedResId = SeqResourceNameHash(data.mSeqName);
            gSeqIdToName[data.mGeneratedResId] = data.mSeqName;
            LOG_INFO(data.mSeqName);
           // LOG_INFO(data.mSeqName << " " << std::hex << data.mGeneratedResId);
        }
    }

    //u32 resId = SeqResourceNameHash("OPTAMB.SEQ");
    //assert(resId == 0x0DCD80);

}

struct SeqStruct
{
    BYTE* mSeqDataPtr;
    DWORD field_4;
    DWORD field_8;
    DWORD field_C_fixed;
    DWORD field_10;
    DWORD unknown1;
    DWORD unknown2;
    BYTE* mSeqStartDataPtr;
    DWORD data[17];
};
static_assert(sizeof(SeqStruct) == 100, "Wrong size SeqStruct");

SeqStruct* g_seq_data_dword_C13400 = (SeqStruct*)0xC13400;

struct SeqHeader
{
    u32 magic;
    u32 version;
    u16 resoultionOfQuaterNote;
    u8 tempo[3];
    u8 rythmNumerator;
    u8 rythnDemoninator;
};

bool gNoMusic = true;

// 0x004FD100 
signed int __cdecl SND_seq_play_q(int aSeqIndx)
{
    if (gNoMusic)
    {
        return 0;
    }

    // The index seems to be an index into an internal array - I can't figure out how the heck it
    // works but the structure of what is in the array appears to be known, so look up the SEQ data
    // from the item in the array at aSeqIndx and then look up the SEQ name from its ID.

    SeqStruct* pData = g_seq_data_dword_C13400 + aSeqIndx;

    if (pData->mSeqStartDataPtr)
    {
        // This points to the start of the seq data, so we have to back by the size of SEQ header + length of id
        BYTE* ptr = pData->mSeqStartDataPtr - sizeof(SeqHeader) - sizeof(u32) + 1;
        DWORD id = *((DWORD*)ptr);
        auto it = gSeqIdToName.find(id);
        if (it != std::end(gSeqIdToName))
        {
            LOG_INFO("Playing: " << it->second);
        }
        else
        {
            LOG_ERROR("UNKNOWN SEQ");
        }
    }


    return SND_seq_play_q_hook.Real()(aSeqIndx);
}
