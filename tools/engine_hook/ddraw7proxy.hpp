#pragma once

#include <windows.h>
#include <ddraw.h>
#include "dsurface7proxy.hpp"

class DirectDraw7Proxy : public IDirectDraw
{
private:
    IDirectDraw* mDDraw = nullptr;
    HWND mHwnd = 0;
public:
    DirectDraw7Proxy(IDirectDraw* pDDraw)
        : mDDraw(pDDraw)
    {

    }

    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR * ppvObj)
    {
        return mDDraw->QueryInterface(riid, ppvObj);
    }

    STDMETHOD_(ULONG, AddRef) (THIS)
    {
        return mDDraw->AddRef();
    }

    STDMETHOD_(ULONG, Release) (THIS)
    {
        return mDDraw->Release();
    }

    /*** IDirectDraw methods ***/
    STDMETHOD(Compact)(THIS)
    {
        return mDDraw->Compact();
    }

    STDMETHOD(CreateClipper)(THIS_ DWORD a1, LPDIRECTDRAWCLIPPER FAR* a2, IUnknown FAR * a3)
    {
        return mDDraw->CreateClipper(a1, a2, a3);
    }

    STDMETHOD(CreatePalette)(THIS_ DWORD a1, LPPALETTEENTRY a2, LPDIRECTDRAWPALETTE FAR* a3, IUnknown FAR * a4)
    {
        return mDDraw->CreatePalette(a1, a2, a3, a4);
    }

    void Set16bit(LPDDSURFACEDESC lpDDSurfaceDesc)
    {
        lpDDSurfaceDesc->dwFlags |= DDSD_PIXELFORMAT;
        lpDDSurfaceDesc->ddpfPixelFormat.dwSize = 0x20; // Hack for V7
        lpDDSurfaceDesc->ddpfPixelFormat.dwFlags = DDPF_RGB;
        lpDDSurfaceDesc->ddpfPixelFormat.dwFourCC = 0;
        lpDDSurfaceDesc->ddpfPixelFormat.dwRGBBitCount = 16;
        lpDDSurfaceDesc->ddpfPixelFormat.dwYUVBitCount = 16;
        lpDDSurfaceDesc->ddpfPixelFormat.dwZBufferBitDepth = 16;
        lpDDSurfaceDesc->ddpfPixelFormat.dwAlphaBitDepth = 16;
        lpDDSurfaceDesc->ddpfPixelFormat.dwLuminanceBitCount = 16;
        lpDDSurfaceDesc->ddpfPixelFormat.dwBumpBitCount = 16;

        lpDDSurfaceDesc->ddpfPixelFormat.dwRBitMask = 63488;
        lpDDSurfaceDesc->ddpfPixelFormat.dwYBitMask = 63488;
        lpDDSurfaceDesc->ddpfPixelFormat.dwStencilBitDepth = 63488;
        lpDDSurfaceDesc->ddpfPixelFormat.dwLuminanceBitMask = 63488;
        lpDDSurfaceDesc->ddpfPixelFormat.dwBumpDuBitMask = 63488;
        lpDDSurfaceDesc->ddpfPixelFormat.dwGBitMask = 2016;
        lpDDSurfaceDesc->ddpfPixelFormat.dwUBitMask = 2016;
        lpDDSurfaceDesc->ddpfPixelFormat.dwZBitMask = 2016;
        lpDDSurfaceDesc->ddpfPixelFormat.dwBumpDvBitMask = 2016;


        lpDDSurfaceDesc->ddpfPixelFormat.dwBBitMask = 31;
        lpDDSurfaceDesc->ddpfPixelFormat.dwVBitMask = 31;
        lpDDSurfaceDesc->ddpfPixelFormat.dwStencilBitMask = 31;
        lpDDSurfaceDesc->ddpfPixelFormat.dwBumpLuminanceBitMask = 31;
        lpDDSurfaceDesc->ddpfPixelFormat.dwRGBAlphaBitMask = 0;
        lpDDSurfaceDesc->ddpfPixelFormat.dwYUVAlphaBitMask = 0;
        lpDDSurfaceDesc->ddpfPixelFormat.dwLuminanceAlphaBitMask = 0;
        lpDDSurfaceDesc->ddpfPixelFormat.dwRGBZBitMask = 0;
        lpDDSurfaceDesc->ddpfPixelFormat.dwYUVZBitMask = 0;

        lpDDSurfaceDesc->ddpfPixelFormat.dwPrivateFormatBitCount = 16;
        lpDDSurfaceDesc->ddpfPixelFormat.MultiSampleCaps.wFlipMSTypes = 2016;
        lpDDSurfaceDesc->ddpfPixelFormat.MultiSampleCaps.wBltMSTypes = 0;
        lpDDSurfaceDesc->ddpfPixelFormat.dwOperations = 63488;
    }

    STDMETHOD(CreateSurface)
        (THIS_  LPDDSURFACEDESC lpDDSurfaceDesc,
            LPDIRECTDRAWSURFACE FAR * lplpDDSurface,
            IUnknown FAR * pUnkOuter)
    {
        if (lpDDSurfaceDesc->dwFlags & DDSD_CAPS)
        {
            lpDDSurfaceDesc->dwFlags = lpDDSurfaceDesc->dwFlags & ~DDSD_BACKBUFFERCOUNT;
            lpDDSurfaceDesc->dwBackBufferCount = 0;

            if (lpDDSurfaceDesc->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
            {
                // Set ONLY DDSCAPS_PRIMARYSURFACE
                lpDDSurfaceDesc->ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
            }
            else
            {
                Set16bit(lpDDSurfaceDesc);
            }
        }

        // Really create it
        HRESULT ret =  mDDraw->CreateSurface(lpDDSurfaceDesc, lplpDDSurface, pUnkOuter);

        // Create a "wrapper" for it
        DirectSurface7Proxy* hookSurf = new DirectSurface7Proxy(mHwnd, (IDirectDrawSurface7 *)*lplpDDSurface);

        // Return the wrapper
        *lplpDDSurface = (LPDIRECTDRAWSURFACE)hookSurf;

        if (lpDDSurfaceDesc->dwWidth == 0)
        {
            hookSurf->SetName("Primary");

            // Create a new 
            static DDSURFACEDESC desc = { 0 };
            desc.dwSize = sizeof(DDSURFACEDESC);
            desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
            desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
            desc.dwWidth = 640;
            desc.dwHeight = 480;

            Set16bit(&desc);

            /*HRESULT ret =*/ mDDraw->CreateSurface(&desc, lplpDDSurface, pUnkOuter);

            DirectSurface7Proxy*  hookFakeBackBuffer = new DirectSurface7Proxy(mHwnd, (IDirectDrawSurface7 *)*lplpDDSurface);
            hookFakeBackBuffer->SetName("FakePrimary");

            // Back buffer points to the "real" back buffer
            DirectSurface7Proxy::g_Primary = hookSurf;

            // Set up a clipper on g_Primary
            static LPDIRECTDRAWCLIPPER clipper;
            /*HRESULT clipRet =*/ CreateClipper(0, &clipper, NULL);
            clipper->SetHWnd(0, mHwnd);
            DirectSurface7Proxy::g_Primary->mRealSurface->SetClipper(clipper);


            // "Fake" back buffer which the game thinks is the "real" one
            DirectSurface7Proxy::g_FakePrimary = hookFakeBackBuffer;

            // Give the game the "fake" back buffer as the real one
            *lplpDDSurface = (LPDIRECTDRAWSURFACE)hookFakeBackBuffer;

        }
        /*
        if ( lpDDSurfaceDesc->dwWidth == 480 )
        {
        hookSurf->SetName("Screen");
        IDirectDrawSurface71::g_Primary = hookSurf;
        }
        */
        if (lpDDSurfaceDesc->dwWidth == 1024)
        {
            hookSurf->SetName("PsxEmu");
            DirectSurface7Proxy::g_BackBuffer = hookSurf;
        }

        return ret;
    }

    STDMETHOD(DuplicateSurface)(THIS_ LPDIRECTDRAWSURFACE a1, LPDIRECTDRAWSURFACE FAR * a2)
    {
        return mDDraw->DuplicateSurface(a1, a2);
    }

    STDMETHOD(EnumDisplayModes)(THIS_ DWORD a1, LPDDSURFACEDESC a2, LPVOID a3, LPDDENUMMODESCALLBACK a4)
    {
        return mDDraw->EnumDisplayModes(a1, a2, a3, a4);
    }

    STDMETHOD(EnumSurfaces)(THIS_ DWORD a1, LPDDSURFACEDESC a2, LPVOID a3, LPDDENUMSURFACESCALLBACK a4)
    {
        return mDDraw->EnumSurfaces(a1, a2, a3, a4);
    }

    STDMETHOD(FlipToGDISurface)(THIS)
    {
        return mDDraw->FlipToGDISurface();
    }

    STDMETHOD(GetCaps)(THIS_ LPDDCAPS a1, LPDDCAPS a2)
    {
        return mDDraw->GetCaps(a1, a2);
    }

    STDMETHOD(GetDisplayMode)(THIS_ LPDDSURFACEDESC a1)
    {
        return mDDraw->GetDisplayMode(a1);
    }

    STDMETHOD(GetFourCCCodes)(THIS_  LPDWORD a1, LPDWORD a2)
    {
        return mDDraw->GetFourCCCodes(a1, a2);
    }

    STDMETHOD(GetGDISurface)(THIS_ LPDIRECTDRAWSURFACE FAR * a2)
    {
        return mDDraw->GetGDISurface(a2);
    }

    STDMETHOD(GetMonitorFrequency)(THIS_ LPDWORD a1)
    {
        return mDDraw->GetMonitorFrequency(a1);
    }

    STDMETHOD(GetScanLine)(THIS_ LPDWORD a1)
    {
        return mDDraw->GetScanLine(a1);
    }

    STDMETHOD(GetVerticalBlankStatus)(THIS_ LPBOOL a1)
    {
        return mDDraw->GetVerticalBlankStatus(a1);
    }

    STDMETHOD(Initialize)(THIS_ GUID FAR * a1)
    {
        return mDDraw->Initialize(a1);
    }

    STDMETHOD(RestoreDisplayMode)(THIS)
    {
        return mDDraw->RestoreDisplayMode();
    }

    STDMETHOD(SetCooperativeLevel)(THIS_ HWND aHwnd, DWORD aFlags)
    {
        mHwnd = aHwnd;

        // Force normal so we can run in a window
        aFlags = DDSCL_NORMAL;

        return mDDraw->SetCooperativeLevel(aHwnd, aFlags);
    }

    STDMETHOD(SetDisplayMode)(THIS_ DWORD /*a1*/, DWORD /*a2*/, DWORD /*a3*/)
    {
        // Don't do anything, we want to run in a window
        //return DDrawUtils::g_pDD->SetDisplayMode( a1, a2, a3);
        return DD_OK;
    }

    STDMETHOD(WaitForVerticalBlank)(THIS_ DWORD a1, HANDLE a2)
    {
        return mDDraw->WaitForVerticalBlank(a1, a2);
    }

};
