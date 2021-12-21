#pragma once

#include "DXSample.h"
#include "interfaces/engineinterface.h"

#include "StepTimer.h"
#include "simplemath.h"
#include "GameBase.h"
#include "engineutils.h"
#include "graphics/gfxmemory.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

class softbody : public DXSample, public game_engine
{
public:
    softbody(gamedata const& data = {});

    virtual void OnInit();
    virtual void OnUpdate();
    virtual void OnRender();
    virtual void OnDestroy();
    virtual void OnKeyDown(UINT8 key);
    virtual void OnKeyUp(UINT8 key);

private:

    // Synchronization objects.
    HANDLE m_fenceEvent;
    ComPtr<ID3D12Fence> m_fence;
    UINT64 m_fenceValues[configurable_properties::frame_count];

    // Pipeline objects.
    CD3DX12_VIEWPORT m_viewport;
    CD3DX12_RECT m_scissorRect;
    ComPtr<IDXGISwapChain3> m_swapChain;
    ComPtr<ID3D12Resource> m_renderTargets[configurable_properties::frame_count];
    ComPtr<ID3D12Resource> m_depthStencil;
    ComPtr<ID3D12CommandAllocator> m_commandAllocators[configurable_properties::frame_count];
    ComPtr<ID3D12CommandQueue> m_commandQueue;
    ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    ComPtr<ID3D12DescriptorHeap> m_dsvHeap;

    unsigned m_rtvDescriptorSize;
    unsigned m_dsvDescriptorSize;

    StepTimer m_timer;
    std::unique_ptr<game_base> game;

    // todo : is this useful anymore?
    configurable_properties config_properties;

    unsigned m_frameCounter;

    void load_pipeline();
    void load_assetsandgeometry();
    void moveto_nextframe();
    void waitforgpu();
};