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

#include "DXSample.h"
#include "interfaces/engineinterface.h"

#include "StepTimer.h"
#include "SimpleMath.h"
#include "GameBase.h"
#include "engineutils.h"
#include "graphics/gfxmemory.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

class softbody : public DXSample, public game_engine
{
public:
    softbody(configurable_properties const & config_props = configurable_properties());

    virtual void OnInit();
    virtual void OnUpdate();
    virtual void OnRender();
    virtual void OnDestroy();
    virtual void OnKeyDown(UINT8 key);
    virtual void OnKeyUp(UINT8 key);

    // game_engine interface
    unsigned get_frame_index() const override { return m_frameIndex; }
    ID3D12Device2* get_device() const override { return m_device.Get(); }
    ID3D12GraphicsCommandList6* get_command_list() const override { return m_commandList.Get(); }
    configurable_properties const& get_config_properties() const override { return config_properties; }
    std::wstring get_asset_fullpath(std::wstring const& asset_name) const override { return m_assetsPath + asset_name; }     // Helper function for resolving the full path of assets.
    D3DX12_MESH_SHADER_PIPELINE_STATE_DESC get_pso_desc() const override;
    // game_engine interface

private:

    // Synchronization objects.
    HANDLE m_fenceEvent;
    ComPtr<ID3D12Fence> m_fence;
    UINT64 m_fenceValues[configurable_properties::frame_count];

    // Pipeline objects.
    CD3DX12_VIEWPORT m_viewport;
    CD3DX12_RECT m_scissorRect;
    ComPtr<IDXGISwapChain3> m_swapChain;
    ComPtr<ID3D12Device2> m_device;
    ComPtr<ID3D12Resource> m_renderTargets[configurable_properties::frame_count];
    ComPtr<ID3D12Resource> m_depthStencil;
    ComPtr<ID3D12CommandAllocator> m_commandAllocators[configurable_properties::frame_count];
    ComPtr<ID3D12CommandQueue> m_commandQueue;
    ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    ComPtr<ID3D12DescriptorHeap> m_dsvHeap;

    ComPtr<ID3D12GraphicsCommandList6> m_commandList;

    unsigned m_rtvDescriptorSize;
    unsigned m_dsvDescriptorSize;

    StepTimer m_timer;
    std::wstring m_assetsPath;
    std::unique_ptr<game_base> game;
    configurable_properties config_properties;

    unsigned m_frameIndex;
    unsigned m_frameCounter;

    void load_pipeline();
    void load_assetsandgeometry();
    void moveto_nextframe();
    void waitforgpu();
};