#pragma once

#include "dsound.h"
#include "logger.hpp"
#include <set>

extern std::set<class DirectSoundBuffer7Proxy*> gSoundBuffers;
extern class DirectSound7Proxy* gDSound;

class DirectSoundBuffer7Proxy : public IDirectSoundBuffer
{
public:

    DirectSoundBuffer7Proxy(IDirectSoundBuffer& realSnd)
        : mRealSnd(realSnd), mRefCount(0)
    {
        gSoundBuffers.insert(this);
    }

    virtual ~DirectSoundBuffer7Proxy()
    {
        gSoundBuffers.erase(this);
    }

    // IUnknown methods
    STDMETHOD(QueryInterface)       (THIS_ REFIID id, LPVOID FAR * ptr)
    {
        TRACE_ENTRYEXIT;
        return mRealSnd.QueryInterface(id, ptr);
    }

    STDMETHOD_(ULONG, AddRef)        (THIS)
    {
        TRACE_ENTRYEXIT;
        mRefCount++;
        return mRealSnd.AddRef();
    }

    STDMETHOD_(ULONG, Release)       (THIS)
    {
        TRACE_ENTRYEXIT;
        mRefCount--;
        HRESULT ret = mRealSnd.Release();
        if (mRefCount <= 0)
        {
            delete this;
        }
        return ret;
    }

    // IDirectSoundBuffer methods
    STDMETHOD(GetCaps)              (THIS_ LPDSBCAPS caps)
    {
        TRACE_ENTRYEXIT;
        return mRealSnd.GetCaps(caps);
    }

    STDMETHOD(GetCurrentPosition)   (THIS_ LPDWORD a1, LPDWORD a2)
    {
        TRACE_ENTRYEXIT;
        return mRealSnd.GetCurrentPosition(a1, a2);
    }

    STDMETHOD(GetFormat)            (THIS_ LPWAVEFORMATEX f, DWORD a1, LPDWORD a2)
    {
        TRACE_ENTRYEXIT;
        return mRealSnd.GetFormat(f, a1, a2);
    }

    STDMETHOD(GetVolume)            (THIS_ LPLONG a1)
    {
        return mRealSnd.GetVolume(a1);
    }

    STDMETHOD(GetPan)               (THIS_ LPLONG a1)
    {
        return mRealSnd.GetPan(a1);
    }

    STDMETHOD(GetFrequency)         (THIS_ LPDWORD a1)
    {
        return mRealSnd.GetFrequency(a1);
    }

    STDMETHOD(GetStatus)            (THIS_ LPDWORD a1)
    {
        return mRealSnd.GetStatus(a1);
    }

    STDMETHOD(Initialize)           (THIS_ LPDIRECTSOUND a1, LPCDSBUFFERDESC a2)
    {
        TRACE_ENTRYEXIT;
        return mRealSnd.Initialize(a1, a2);
    }

    STDMETHOD(Lock)                 (THIS_ DWORD a1, DWORD a2, LPVOID * a3, LPDWORD a4, LPVOID * a5, LPDWORD a6, DWORD a7)
    {
        return mRealSnd.Lock(a1, a2, a3, a4, a5, a6, a7);
    }

    STDMETHOD(Play)                 (THIS_ DWORD dwReserved1, DWORD dwPriority, DWORD dwFlags)
    {
        TRACE_ENTRYEXIT;
        HRESULT ret = mRealSnd.Play(dwReserved1, dwPriority, dwFlags);
        switch (ret)
        {
        case DSERR_BUFFERLOST:
            break;

        case DSERR_INVALIDCALL:
            break;
        case DSERR_INVALIDPARAM:
            break;

        case DSERR_PRIOLEVELNEEDED:
            break;
        }
        return ret;
    }

    STDMETHOD(SetCurrentPosition)   (THIS_ DWORD a1)
    {
        TRACE_ENTRYEXIT;
        return mRealSnd.SetCurrentPosition(a1);
    }

    STDMETHOD(SetFormat)            (THIS_ LPCWAVEFORMATEX f)
    {
        return mRealSnd.SetFormat(f);
    }

    STDMETHOD(SetVolume)            (THIS_ LONG a)
    {
        return mRealSnd.SetVolume(a);
    }

    STDMETHOD(SetPan)               (THIS_ LONG a)
    {
        return mRealSnd.SetPan(a);
    }

    STDMETHOD(SetFrequency)         (THIS_ DWORD a)
    {
        //TRACE_ENTRYEXIT;

        /*
        switch(a)
        {
        case 1840:
        case 5513:
        case 1460:
        case 10406:
        case 9271:
        a = 44100;
        break;
        }
        */

        //a = 22100;
        LOG_INFO("Freq: " << a);
        return mRealSnd.SetFrequency(a);
    }

    STDMETHOD(Stop)                 (THIS)
    {
        return mRealSnd.Stop();
    }

    STDMETHOD(Unlock)               (THIS_ LPVOID a1, DWORD a2, LPVOID a3, DWORD a4)
    {
        return mRealSnd.Unlock(a1, a2, a3, a4);
    }

    STDMETHOD(Restore)              (THIS)
    {
        TRACE_ENTRYEXIT;
        return mRealSnd.Restore();
    }

    IDirectSoundBuffer& mRealSnd;
    int mRefCount = 0;
};

class DirectSound7Proxy : public IDirectSound
{
public:
    DirectSound7Proxy(IDirectSound& aRealSound)
        : mDSound7(aRealSound)
    {
        gDSound = this;
    }

    // IUnknown methods
    STDMETHOD(QueryInterface)       (THIS_ REFIID aId, LPVOID FAR * aVoid)
    {
        return mDSound7.QueryInterface(aId, aVoid);
    }

    STDMETHOD_(ULONG, AddRef)        (THIS)
    {
        return mDSound7.AddRef();
    }

    STDMETHOD_(ULONG, Release)       (THIS)
    {
        return mDSound7.Release();
    }

    // IDirectSound methods
    STDMETHOD(CreateSoundBuffer)    (THIS_ LPCDSBUFFERDESC aDesc, LPDIRECTSOUNDBUFFER * aBuffer, LPUNKNOWN aUnknown)
    {
        if (aDesc)
        {
            // If its not the primary buffer
            if (!(aDesc->dwFlags & DSBCAPS_PRIMARYBUFFER))
            {
                // Set DSBCAPS_GLOBALFOCUS so that sounds keep playing
                // when the window looses focus.
                ((_DSBUFFERDESC*)aDesc)->dwFlags |= DSBCAPS_GLOBALFOCUS;
            }
        }
        HRESULT ret = mDSound7.CreateSoundBuffer(aDesc, aBuffer, aUnknown);

        // Put the real in a wrapper
        DirectSoundBuffer7Proxy* tmp = new DirectSoundBuffer7Proxy(**aBuffer);

        // And return the wrapper to the caller
        *aBuffer = tmp;

        return ret;
    }

    STDMETHOD(GetCaps)              (THIS_ LPDSCAPS aCaps)
    {
        return mDSound7.GetCaps(aCaps);
    }

    STDMETHOD(DuplicateSoundBuffer) (THIS_ LPDIRECTSOUNDBUFFER aBuffer, LPDIRECTSOUNDBUFFER * aBufferPtr)
    {
        DirectSoundBuffer7Proxy* ptr = (DirectSoundBuffer7Proxy*)aBuffer;


        LPDIRECTSOUNDBUFFER ptrToRealBuffer = &ptr->mRealSnd;

        HRESULT ret = mDSound7.DuplicateSoundBuffer(ptrToRealBuffer, aBufferPtr);

        DirectSoundBuffer7Proxy* wrapper = new DirectSoundBuffer7Proxy(**aBufferPtr);

        *aBufferPtr = wrapper;

        return ret;
    }

    STDMETHOD(SetCooperativeLevel)  (THIS_ HWND aHwnd, DWORD aFlags)
    {
        return mDSound7.SetCooperativeLevel(aHwnd, aFlags);
    }

    STDMETHOD(Compact)              (THIS)
    {
        return mDSound7.Compact();
    }

    STDMETHOD(GetSpeakerConfig)     (THIS_ LPDWORD aConf)
    {
        return mDSound7.GetSpeakerConfig(aConf);
    }

    STDMETHOD(SetSpeakerConfig)     (THIS_ DWORD aConf)
    {
        return mDSound7.SetSpeakerConfig(aConf);
    }

    STDMETHOD(Initialize)           (THIS_ _In_opt_ LPCGUID pcGuidDevice)
    {
        return mDSound7.Initialize(pcGuidDevice);
    }


private:
    IDirectSound& mDSound7;
};
