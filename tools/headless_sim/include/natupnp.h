#pragma once
// Lightweight mock for natupnp.h on host headless builds
#include <windows.h>
#include <unknwn.h>
#include <oleauto.h>

struct DECLSPEC_UUID("6F10795F-37EC-41e5-B623-EE757940FF86") IStaticPortMapping : public IUnknown {
    virtual HRESULT STDMETHODCALLTYPE get_ExternalIPAddress(BSTR*) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_ExternalPort(long*) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_InternalPort(long*) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_Protocol(BSTR*) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_InternalClient(BSTR*) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_Enabled(VARIANT_BOOL*) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_Description(BSTR*) = 0;
    virtual HRESULT STDMETHODCALLTYPE EditInternalClient(BSTR) = 0;
    virtual HRESULT STDMETHODCALLTYPE Enable(VARIANT_BOOL) = 0;
    virtual HRESULT STDMETHODCALLTYPE EditDescription(BSTR) = 0;
    virtual HRESULT STDMETHODCALLTYPE EditInternalPort(long) = 0;
};

struct DECLSPEC_UUID("CD1F3EEB-B40E-4342-9214-E431E66182E4") IStaticPortMappingCollection : public IUnknown {
    virtual HRESULT STDMETHODCALLTYPE get_Item(long, BSTR, IStaticPortMapping**) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_Count(long*) = 0;
    virtual HRESULT STDMETHODCALLTYPE Remove(long, BSTR) = 0;
    virtual HRESULT STDMETHODCALLTYPE Add(long, BSTR, long, BSTR, VARIANT_BOOL, BSTR, IStaticPortMapping**) = 0;
    virtual HRESULT STDMETHODCALLTYPE get__NewEnum(IUnknown**) = 0;
};

struct DECLSPEC_UUID("B2EED591-D84D-47CD-84B9-450F2C247925") IUPnPNAT : public IUnknown {
    virtual HRESULT STDMETHODCALLTYPE get_StaticPortMappingCollection(IStaticPortMappingCollection**) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_DynamicPortMappingCollection(void**) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_NATEventManager(void**) = 0;
};

class DECLSPEC_UUID("AE1E00AA-3435-4644-8A68-09033737DC20") UPnPNAT;
