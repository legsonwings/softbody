//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers.
#endif

#define NOMINMAX
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include "d3dx12.h"

#include <wrl.h>
#include <shellapi.h>

#include "DXSampleHelper.h"
#include "Win32Application.h"

class DXSample
{
public:
    DXSample(UINT width, UINT height);
    virtual ~DXSample();

    virtual void OnInit() = 0;
    virtual void OnUpdate() = 0;
    virtual void OnRender() = 0;
    virtual void OnDestroy() = 0;

    // Samples override the event handlers to handle specific messages.
    virtual void OnKeyDown(UINT8 /*key*/)   {}
    virtual void OnKeyUp(UINT8 /*key*/)     {}

    // Accessors.
    UINT GetWidth() const           { return m_width; }
    UINT GetHeight() const          { return m_height; }
    const WCHAR* GetTitle() const   { return m_title.c_str(); }

    void ParseCommandLineArgs(_In_reads_(argc) WCHAR* argv[], int argc);

protected:

    void GetHardwareAdapter(
        _In_ IDXGIFactory1* pFactory,
        _Outptr_result_maybenull_ IDXGIAdapter1** ppAdapter,
        bool requestHighPerformanceAdapter = false);

    void SetCustomWindowText(std::wstring const& text);

    // Viewport dimensions.
    UINT m_width;
    UINT m_height;

    // Adapter info.
    bool m_useWarpDevice;

private:


    // Window title.
    std::wstring m_title;
};
