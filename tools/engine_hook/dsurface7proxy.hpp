#pragma once

#include <sstream>
#include <fstream>
#include <ddraw.h>


void RenderToProxySurface(HWND hwnd);

class DirectSurface7Proxy : public IDirectDrawSurface7
{
public:
    static DirectSurface7Proxy* g_Primary;
    static DirectSurface7Proxy* g_BackBuffer;
    static DirectSurface7Proxy* g_FakePrimary;

public:

    DirectSurface7Proxy(HWND hwnd, IDirectDrawSurface7* aRealSurface)
        : mRealSurface(aRealSurface), mName("Unknown"), mHwnd(hwnd)
    {
    }

    virtual ~DirectSurface7Proxy() { }

    void SetName(const char* aName)
    {
        mName = aName;
    }

    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR * ppvObj)
    {
        return mRealSurface->QueryInterface(riid, ppvObj);
    }

    STDMETHOD_(ULONG, AddRef) (THIS)
    {
        return mRealSurface->AddRef();
    }

    STDMETHOD_(ULONG, Release) (THIS)
    {
        // return mRealSurface->Release();
        // TODO: We also need to delete this!
        return S_OK;
    }

    /*** IDirectDrawSurface methods ***/
    STDMETHOD(AddAttachedSurface)(THIS_ LPDIRECTDRAWSURFACE7 a1)
    {
        return mRealSurface->AddAttachedSurface(a1);
    }

    STDMETHOD(AddOverlayDirtyRect)(THIS_ LPRECT a1)
    {
        return mRealSurface->AddOverlayDirtyRect(a1);
    }

    STDMETHOD(Blt)(THIS_ LPRECT a1, LPDIRECTDRAWSURFACE7 a2, LPRECT a3, DWORD a4, LPDDBLTFX a5)
    {
        //return DD_OK;

        return mRealSurface->Blt(a1, a2, a3, a4, a5);
    }

    STDMETHOD(BltBatch)(THIS_ LPDDBLTBATCH a1, DWORD a2, DWORD a3)
    {
        return mRealSurface->BltBatch(a1, a2, a3);
    }

    STDMETHOD(BltFast)(THIS_ DWORD a1, DWORD a2, LPDIRECTDRAWSURFACE7 a3, LPRECT a4, DWORD a5)
    {
        a5 = DDBLTFAST_WAIT;

        DirectSurface7Proxy* surf = (DirectSurface7Proxy*)a3;

        HRESULT ret = mRealSurface->BltFast(a1, a2, surf->mRealSurface, a4, a5);
        return ret;
    }

    STDMETHOD(DeleteAttachedSurface)(THIS_ DWORD a1, LPDIRECTDRAWSURFACE7 a2)
    {
        return mRealSurface->DeleteAttachedSurface(a1, a2);
    }

    STDMETHOD(EnumAttachedSurfaces)(THIS_ LPVOID a1, LPDDENUMSURFACESCALLBACK7 a2)
    {
        return mRealSurface->EnumAttachedSurfaces(a1, a2);
    }

    STDMETHOD(EnumOverlayZOrders)(THIS_ DWORD a1, LPVOID a2, LPDDENUMSURFACESCALLBACK7 a3)
    {
        return mRealSurface->EnumOverlayZOrders(a1, a2, a3);
    }

    STDMETHOD(Flip)(THIS_ LPDIRECTDRAWSURFACE7 a1, DWORD a2)
    {
        return mRealSurface->Flip(a1, a2);
    }

    STDMETHOD(GetAttachedSurface)(THIS_ LPDDSCAPS2 a1, LPDIRECTDRAWSURFACE7 FAR * a2)
    {
        HRESULT ret = mRealSurface->GetAttachedSurface(a1, a2);
        if (!*a2)
        {
            *a2 = (LPDIRECTDRAWSURFACE7)this;
            ret = DD_OK;
        }
        return ret;
    }

    STDMETHOD(GetBltStatus)(THIS_ DWORD a1)
    {
        return mRealSurface->GetBltStatus(a1);
    }

    STDMETHOD(GetCaps)(THIS_ LPDDSCAPS2 a1)
    {
        return mRealSurface->GetCaps(a1);
    }

    STDMETHOD(GetClipper)(THIS_ LPDIRECTDRAWCLIPPER FAR* a1)
    {
        return mRealSurface->GetClipper(a1);
    }

    STDMETHOD(GetColorKey)(THIS_ DWORD a1, LPDDCOLORKEY a2)
    {
        return mRealSurface->GetColorKey(a1, a2);
    }

    STDMETHOD(GetDC)(THIS_ HDC FAR * a1)
    {
        return mRealSurface->GetDC(a1);
    }

    STDMETHOD(GetFlipStatus)(THIS_ DWORD a1)
    {
        return mRealSurface->GetFlipStatus(a1);
    }

    STDMETHOD(GetOverlayPosition)(THIS_ LPLONG a1, LPLONG a2)
    {
        return mRealSurface->GetOverlayPosition(a1, a2);
    }

    STDMETHOD(GetPalette)(THIS_ LPDIRECTDRAWPALETTE FAR* a1)
    {
        return mRealSurface->GetPalette(a1);
    }

    STDMETHOD(GetPixelFormat)(THIS_ LPDDPIXELFORMAT a1)
    {
        return mRealSurface->GetPixelFormat(a1);
    }

    STDMETHOD(GetSurfaceDesc)(THIS_ LPDDSURFACEDESC2 a1)
    {
        return mRealSurface->GetSurfaceDesc(a1);
    }

    STDMETHOD(Initialize)(THIS_ LPDIRECTDRAW a1, LPDDSURFACEDESC2 a2)
    {
        return mRealSurface->Initialize(a1, a2);
    }

    STDMETHOD(IsLost)(THIS)
    {
        return mRealSurface->IsLost();
    }

    STDMETHOD(Lock)(THIS_ LPRECT a1, LPDDSURFACEDESC2 a2, DWORD a3, HANDLE a4)
    {
        a1 = NULL;
        if (mName == "BackBuffer")
        {
            if (g_Primary && g_BackBuffer)
            {

            }
            //return DD_OK;
        }

        HRESULT ret = mRealSurface->Lock(a1, a2, a3, a4);

        if (mName == "PsxFrameBuffer_1024x512")
        {

        }
        return ret;
    }

    STDMETHOD(ReleaseDC)(THIS_ HDC a1)
    {
        return mRealSurface->ReleaseDC(a1);
    }

    STDMETHOD(Restore)(THIS)
    {
        // The game doesn't know about our extra surface, thus it never gets restored
        // and the screen stays black forever.
        if (g_Primary && g_FakePrimary && g_BackBuffer)
        {
            g_Primary->mRealSurface->Restore();
            g_FakePrimary->mRealSurface->Restore();
            g_BackBuffer->mRealSurface->Restore();
        }
        return mRealSurface->Restore();
    }

    STDMETHOD(SetClipper)(THIS_ LPDIRECTDRAWCLIPPER a1)
    {
        return mRealSurface->SetClipper(a1);
    }

    STDMETHOD(SetColorKey)(THIS_ DWORD a1, LPDDCOLORKEY a2)
    {
        return mRealSurface->SetColorKey(a1, a2);
    }

    STDMETHOD(SetOverlayPosition)(THIS_ LONG a1, LONG a2)
    {
        return mRealSurface->SetOverlayPosition(a1, a2);
    }

    STDMETHOD(SetPalette)(THIS_ LPDIRECTDRAWPALETTE a1)
    {
        return mRealSurface->SetPalette(a1);
    }

    STDMETHOD(Unlock)(THIS_ LPRECT a1)
    {
        a1 = NULL;
        HRESULT ret = mRealSurface->Unlock(a1);
        if (mName == "FakePrimary")
        {
            RenderToProxySurface(mHwnd);
        }
        return ret;
    }

    STDMETHOD(UpdateOverlay)(THIS_ LPRECT a1, LPDIRECTDRAWSURFACE7 a2, LPRECT a3, DWORD a4, LPDDOVERLAYFX a5)
    {
        return mRealSurface->UpdateOverlay(a1, a2, a3, a4, a5);
    }

    STDMETHOD(UpdateOverlayDisplay)(THIS_ DWORD a1)
    {
        return mRealSurface->UpdateOverlayDisplay(a1);
    }

    STDMETHOD(UpdateOverlayZOrder)(THIS_ DWORD a1, LPDIRECTDRAWSURFACE7 a2)
    {
        return mRealSurface->UpdateOverlayZOrder(a1, a2);
    }

    /*** Added in the v2 interface ***/
    STDMETHOD(GetDDInterface)(THIS_ LPVOID FAR * a1)
    {
        return mRealSurface->GetDDInterface(a1);
    }

    STDMETHOD(PageLock)(THIS_ DWORD a1)
    {
        return mRealSurface->PageLock(a1);
    }

    STDMETHOD(PageUnlock)(THIS_ DWORD a1)
    {
        return mRealSurface->PageUnlock(a1);
    }

    /*** Added in the v3 interface ***/
    STDMETHOD(SetSurfaceDesc)(THIS_ LPDDSURFACEDESC2 a1, DWORD a2)
    {
        return mRealSurface->SetSurfaceDesc(a1, a2);
    }

    /*** Added in the v4 interface ***/
    STDMETHOD(SetPrivateData)(THIS_ REFGUID a1, LPVOID a2, DWORD a3, DWORD a4)
    {
        return mRealSurface->SetPrivateData(a1, a2, a3, a4);
    }

    STDMETHOD(GetPrivateData)(THIS_ REFGUID a1, LPVOID a2, LPDWORD a3)
    {
        return mRealSurface->GetPrivateData(a1, a2, a3);
    }

    STDMETHOD(FreePrivateData)(THIS_ REFGUID a1)
    {
        return mRealSurface->FreePrivateData(a1);
    }

    STDMETHOD(GetUniquenessValue)(THIS_ LPDWORD a1)
    {
        return mRealSurface->GetUniquenessValue(a1);
    }

    STDMETHOD(ChangeUniquenessValue)(THIS)
    {
        return mRealSurface->ChangeUniquenessValue();
    }

    /*** Moved Texture7 methods here ***/
    STDMETHOD(SetPriority)(THIS_ DWORD a1)
    {
        return mRealSurface->SetPriority(a1);
    }

    STDMETHOD(GetPriority)(THIS_ LPDWORD a1)
    {
        return mRealSurface->GetPriority(a1);
    }

    STDMETHOD(SetLOD)(THIS_ DWORD a1)
    {
        return mRealSurface->SetLOD(a1);
    }

    STDMETHOD(GetLOD)(THIS_ LPDWORD a1)
    {
        return mRealSurface->GetLOD(a1);
    }

public:
    IDirectDrawSurface7* mRealSurface;
    std::string mName;
    HWND mHwnd;
 //   DDSURFACEDESC2 mDesc;
};

inline void RenderToProxySurface(HWND hwnd)
{
    if (DirectSurface7Proxy::g_Primary && DirectSurface7Proxy::g_BackBuffer && DirectSurface7Proxy::g_FakePrimary)
    {

        POINT p = { 0 };
        ClientToScreen(hwnd, &p);

        RECT rcRectDest;
        GetClientRect(hwnd, &rcRectDest);
        //SetRect(&rcRectDest, 0, 0, 200, 200);
        OffsetRect(&rcRectDest, p.x, p.y);

        RECT rcRectSrc;
        SetRect(&rcRectSrc, 0, 0, 640, 480);

        // Copy g_FakePrimary (used by the game) to g_Primary ( only used by us) at the right location
        /*HRESULT r  =*/ DirectSurface7Proxy::g_Primary->mRealSurface->Blt(&rcRectDest, DirectSurface7Proxy::g_FakePrimary->mRealSurface, &rcRectSrc, DDBLT_WAIT, NULL);
    }
}
