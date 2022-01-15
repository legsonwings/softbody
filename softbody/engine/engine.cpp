#include "engine.h"
#include "gameutils.h"
#include "engine/graphics/globalresources.h"
#include "engine/sharedconstants.h"

#include <random>
#include <algorithm>

#define CONSOLE_LOGS 0

#if CONSOLE_LOGS
#include <iostream>
#endif

using namespace DirectX::SimpleMath;

softbody::softbody(gamedata const& data)
    : DXSample(data.width, data.height)
    , m_viewport(0.0f, 0.0f, static_cast<float>(data.width), static_cast<float>(data.height))
    , m_scissorRect(0, 0, static_cast<LONG>(data.width), static_cast<LONG>(data.height))
    , m_rtvDescriptorSize(0)
    , m_dsvDescriptorSize(0)
    , m_frameCounter(0)
    , m_fenceEvent{}
    , m_fenceValues{}
{
    game = game_creator::create_instance<currentgame>(data);
}

void softbody::OnInit()
{
#if CONSOLE_LOGS
    AllocConsole();
    FILE* DummyPtr;
    freopen_s(&DummyPtr, "CONOUT$", "w", stdout);
#endif

    load_pipeline();
    load_assetsandgeometry();
}

// Load the rendering pipeline dependencies.
void softbody::load_pipeline()
{
    UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)
    // Enable the debug layer (requires the Graphics Tools "optional feature").
    // NOTE: Enabling the debug layer after device creation will invalidate the active device.
    {
        ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            debugController->EnableDebugLayer();

            // Enable additional debug layers.
            dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
        }
    }
#endif

    ComPtr<IDXGIFactory4> factory;
    ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));

    auto &device = gfx::globalresources::get().device();

    if (m_useWarpDevice)
    {
        ComPtr<IDXGIAdapter> warpAdapter;
        ThrowIfFailed(factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));

        ThrowIfFailed(D3D12CreateDevice(
            warpAdapter.Get(),
            D3D_FEATURE_LEVEL_11_0,
            IID_PPV_ARGS(device.ReleaseAndGetAddressOf())
            ));
    }
    else
    {
        ComPtr<IDXGIAdapter1> hardwareAdapter;
        GetHardwareAdapter(factory.Get(), &hardwareAdapter);

        ThrowIfFailed(D3D12CreateDevice(
            hardwareAdapter.Get(),
            D3D_FEATURE_LEVEL_11_0,
            IID_PPV_ARGS(device.ReleaseAndGetAddressOf())
            ));
    }

    D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = { D3D_SHADER_MODEL_6_5 };
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel)))
        || (shaderModel.HighestShaderModel < D3D_SHADER_MODEL_6_5))
    {
        OutputDebugStringA("ERROR: Shader Model 6.5 is not supported\n");
        throw std::exception("Shader Model 6.5 is not supported");
    }

    D3D12_FEATURE_DATA_D3D12_OPTIONS7 features = {};
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &features, sizeof(features)))
        || (features.MeshShaderTier == D3D12_MESH_SHADER_TIER_NOT_SUPPORTED))
    {
        OutputDebugStringA("ERROR: Mesh Shaders aren't supported!\n");
        throw std::exception("Mesh Shaders aren't supported!");
    }

    // Describe and create the command queue.
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    ThrowIfFailed(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));

    // Describe and create the swap chain.
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = configurable_properties::frame_count;
    swapChainDesc.Width = m_width;
    swapChainDesc.Height = m_height;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;

    ComPtr<IDXGISwapChain1> swapChain;
    ThrowIfFailed(factory->CreateSwapChainForHwnd(
        m_commandQueue.Get(),        // Swap chain needs the queue so that it can force a flush on it.
        Win32Application::GetHwnd(),
        &swapChainDesc,
        nullptr,
        nullptr,
        &swapChain
        ));

    // This sample does not support fullscreen transitions.
    ThrowIfFailed(factory->MakeWindowAssociation(Win32Application::GetHwnd(), DXGI_MWA_NO_ALT_ENTER));

    ThrowIfFailed(swapChain.As(&m_swapChain));
    gfx::globalresources::get().frameindex(m_swapChain->GetCurrentBackBufferIndex());

    // Create descriptor heaps.
    {
        // Describe and create a render target view (RTV) descriptor heap.
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors = configurable_properties::frame_count;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        ThrowIfFailed(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));

        m_rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        // Describe and create a render target view (RTV) descriptor heap.
        D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
        dsvHeapDesc.NumDescriptors = 1;
        dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        ThrowIfFailed(device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap)));

        m_dsvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    }

    // Create frame resources.
    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

        // Create a RTV and a command allocator for each frame.
        for (UINT n = 0; n < configurable_properties::frame_count; n++)
        {
            ThrowIfFailed(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n])));
            device->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtvHandle);
            rtvHandle.Offset(1, m_rtvDescriptorSize);

            ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocators[n])));
        }
    }

    // Create the depth stencil view.
    {
        D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
        depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
        depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

        D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
        depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
        depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
        depthOptimizedClearValue.DepthStencil.Stencil = 0;

        auto heap_props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        auto texture_resource_desc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, m_width, m_height, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
        ThrowIfFailed(device->CreateCommittedResource(
            &heap_props,
            D3D12_HEAP_FLAG_NONE,
            &texture_resource_desc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &depthOptimizedClearValue,
            IID_PPV_ARGS(&m_depthStencil)
        ));

        NAME_D3D12_OBJECT(m_depthStencil);

        device->CreateDepthStencilView(m_depthStencil.Get(), &depthStencilDesc, m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
    }

    D3DX12_MESH_SHADER_PIPELINE_STATE_DESC pso_desc = {};
    pso_desc.NumRenderTargets = 1;
    pso_desc.RTVFormats[0] = m_renderTargets[0]->GetDesc().Format;
    pso_desc.DSVFormat = m_depthStencil->GetDesc().Format;
    pso_desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);    // CW front; cull back
    pso_desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);         // Opaque
    pso_desc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT); // Less-equal depth test w/ writes; no stencil
    pso_desc.SampleMask = UINT_MAX;
    pso_desc.SampleDesc = DefaultSampleDesc();

    gfx::globalresources::get().psodesc(pso_desc);
    gfx::globalresources::get().init();
}

// Load the sample assets.
void softbody::load_assetsandgeometry()
{
    auto device = gfx::globalresources::get().device();
    auto &cmdlist = gfx::globalresources::get().cmdlist();

    // Create the command list. They are created in recording state
    ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocators[gfx::globalresources::get().frameindex()].Get(), nullptr, IID_PPV_ARGS(&cmdlist)));

    // need to keep these alive till data is uploaded to gpu
    std::vector<ComPtr<ID3D12Resource>> const gpu_resources = game->load_assets_and_geometry();
    ThrowIfFailed(cmdlist->Close());

    ID3D12CommandList* ppCommandLists[] = { cmdlist.Get() };
    m_commandQueue->ExecuteCommandLists(1, ppCommandLists);

    // Create synchronization objects and wait until assets have been uploaded to the GPU.
    {
        ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
        m_fenceValues[gfx::globalresources::get().frameindex()]++;

        // Create an event handle to use for frame synchronization.
        m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (m_fenceEvent == nullptr)
        {
            ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
        }

        // Wait for the command list to execute; we are reusing the same command 
        // list in our main loop but for now, we just want to wait for setup to 
        // complete before continuing.
        waitforgpu();
    }
}

void softbody::OnUpdate()
{
    m_timer.Tick(NULL);

    if (m_frameCounter++ % 30 == 0)
    {
        // Update window text with FPS value.
        wchar_t fps[64];
        swprintf_s(fps, L"%ufps", m_timer.GetFramesPerSecond());

        std::wstring const title = gametitles[int(currentgame)] + L": " + fps;
        SetCustomWindowText(title);
    }

    game->update(static_cast<float>(m_timer.GetElapsedSeconds()));
}

// Render the scene.
void softbody::OnRender()
{
    auto const frameidx = gfx::globalresources::get().frameindex();
    auto cmdlist = gfx::globalresources::get().cmdlist();

    // Command list allocators can only be reset when the associated 
    // command lists have finished execution on the GPU; apps should use 
    // fences to determine GPU execution progress.
    ThrowIfFailed(m_commandAllocators[frameidx]->Reset());

    // However, when ExecuteCommandList() is called on a particular command 
    // list, that command list can then be reset at any time and must be before 
    // re-recording.
    ThrowIfFailed(cmdlist->Reset(m_commandAllocators[frameidx].Get(), nullptr));

    // Set necessary state.
    cmdlist->RSSetViewports(1, &m_viewport);
    cmdlist->RSSetScissorRects(1, &m_scissorRect);

    auto resource_transition_rt = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[frameidx].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

    // Indicate that the back buffer will be used as a render target.
    cmdlist->ResourceBarrier(1, &resource_transition_rt);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), static_cast<INT>(frameidx), m_rtvDescriptorSize);
    CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
    cmdlist->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

    // Record commands.
    const float clearColor[] = { 0.254901975f, 0.254901975f, 0.254901975f, 1.f };
    cmdlist->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    cmdlist->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    game->render(static_cast<float>(m_timer.GetElapsedSeconds()));

    auto resource_transition_present = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[frameidx].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

    // Indicate that the back buffer will now be used to present.
    cmdlist->ResourceBarrier(1, &resource_transition_present);

    ThrowIfFailed(cmdlist->Close());

    // Execute the command list.
    ID3D12CommandList* ppCommandLists[] = { cmdlist.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // Present the frame.
    ThrowIfFailed(m_swapChain->Present(1, 0));

    moveto_nextframe();
}

void softbody::OnDestroy()
{
    // Ensure that the GPU is no longer referencing resources that are about to be
    // cleaned up by the destructor.
    waitforgpu();

    CloseHandle(m_fenceEvent);
}

void softbody::OnKeyDown(UINT8 key)
{
    game->on_key_down(key);
}

void softbody::OnKeyUp(UINT8 key)
{
    game->on_key_up(key);
}

// Wait for pending GPU work to complete.
void softbody::waitforgpu()
{
    auto const frameidx = gfx::globalresources::get().frameindex();

    // Schedule a Signal command in the queue.
    ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), m_fenceValues[frameidx]));

    // Wait until the fence has been processed.
    ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValues[frameidx], m_fenceEvent));
    WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);

    // Increment the fence value for the current frame.
    m_fenceValues[frameidx]++;
}

// Prepare to render the next frame.
void softbody::moveto_nextframe()
{
    auto const frameidx = gfx::globalresources::get().frameindex();

    // Schedule a Signal command in the queue.
    const UINT64 currentFenceValue = m_fenceValues[frameidx];
    ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), currentFenceValue));

    // Update the frame index.
    gfx::globalresources::get().frameindex(m_swapChain->GetCurrentBackBufferIndex());

    // If the next frame is not ready to be rendered yet, wait until it is ready.
    if (m_fence->GetCompletedValue() < m_fenceValues[frameidx])
    {
        ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValues[frameidx], m_fenceEvent));
        WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);
    }

    // Set the fence value for the next frame.
    m_fenceValues[frameidx] = currentFenceValue + 1;
}
