#include "stdafx.h"
#include "Body.h"
#include "Engine/DXSampleHelper.h"
#include "Engine/Interfaces/GameEngineInterface.h"

gfx::pipeline_objects gfx::create_pipelineobjects(std::wstring const& as, std::wstring const& ms, std::wstring const& ps)
{
    auto engine = game_engine::g_engine;
    auto device = engine->get_device();
    shader ampshader, meshshader, pixelshader;

    if (!as.empty())
        ReadDataFromFile(engine->get_asset_fullpath(as).c_str(), &ampshader.data, &ampshader.size);

    ReadDataFromFile(engine->get_asset_fullpath(ms).c_str(), &meshshader.data, &meshshader.size);
    ReadDataFromFile(engine->get_asset_fullpath(ps).c_str(), &pixelshader.data, &pixelshader.size);

    pipeline_objects result;

    // Pull root signature from the precompiled mesh shaders.
    ThrowIfFailed(device->CreateRootSignature(0, meshshader.data, meshshader.size, IID_PPV_ARGS(result.root_signature.GetAddressOf())));

    D3DX12_MESH_SHADER_PIPELINE_STATE_DESC pso_desc = engine->get_pso_desc();

    pso_desc.pRootSignature = result.root_signature.Get();

    if (!as.empty()) 
        pso_desc.AS = { ampshader.data, ampshader.size };

    pso_desc.MS = { meshshader.data, meshshader.size };
    pso_desc.PS = { pixelshader.data, pixelshader.size };
    pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    auto psostream = CD3DX12_PIPELINE_MESH_STATE_STREAM(pso_desc);

    D3D12_PIPELINE_STATE_STREAM_DESC stream_desc;
    stream_desc.pPipelineStateSubobjectStream = &psostream;
    stream_desc.SizeInBytes = sizeof(psostream);

    ThrowIfFailed(device->CreatePipelineState(&stream_desc, IID_PPV_ARGS(result.pso.GetAddressOf())));

    // Setup the Wireframe pso
    pso_desc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
    auto psostream_wire = CD3DX12_PIPELINE_MESH_STATE_STREAM(pso_desc);

    stream_desc.pPipelineStateSubobjectStream = &psostream_wire;
    stream_desc.SizeInBytes = sizeof(psostream_wire);

    ThrowIfFailed(device->CreatePipelineState(&stream_desc, IID_PPV_ARGS(result.pso_wireframe.GetAddressOf())));

    return result;
}

gfx::default_and_upload_buffers gfx::create_vertexbuffer_default(void* vertexdata_start, std::size_t const vb_size)
{
    auto device = game_engine::g_engine->get_device();

    ComPtr<ID3D12Resource> vb;
    ComPtr<ID3D12Resource> vb_upload;

    if (vb_size > 0)
    {
        auto vb_desc = CD3DX12_RESOURCE_DESC::Buffer(vb_size);

        // Create vertex buffer on the default heap
        auto defaultheap_desc = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        ThrowIfFailed(device->CreateCommittedResource(&defaultheap_desc, D3D12_HEAP_FLAG_NONE, &vb_desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(vb.ReleaseAndGetAddressOf())));

        // Create vertex resource on the upload heap
        auto uploadheap_desc = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        ThrowIfFailed(device->CreateCommittedResource(&uploadheap_desc, D3D12_HEAP_FLAG_NONE, &vb_desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(vb_upload.GetAddressOf())));

        {
            uint8_t* vb_upload_start = nullptr;

            // We do not intend to read from this resource on the CPU.
            vb_upload->Map(0, nullptr, reinterpret_cast<void**>(&vb_upload_start));

            // Copy vertex data to upload heap
            memcpy(vb_upload_start, vertexdata_start, vb_size);

            vb_upload->Unmap(0, nullptr);
        }

        auto resource_transition = CD3DX12_RESOURCE_BARRIER::Transition(vb.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

        auto cmd_list = game_engine::g_engine->get_command_list();

        // Copy vertex data from upload heap to default heap
        cmd_list->CopyResource(vb.Get(), vb_upload.Get());
        cmd_list->ResourceBarrier(1, &resource_transition);
    }

    return { vb, vb_upload };
}

ComPtr<ID3D12Resource> gfx::create_vertexbuffer_upload(uint8_t** mapped_buffer, void* vertexdata_start, std::size_t const perframe_buffersize)
{
    auto result = create_uploadbuffer(mapped_buffer, configurable_properties::frame_count * perframe_buffersize);
    update_perframebuffer(*mapped_buffer, vertexdata_start, perframe_buffersize);

    return result;
}

ComPtr<ID3D12Resource> gfx::create_instancebuffer(uint8_t** mapped_buffer, void* data_start, std::size_t const perframe_buffersize)
{
    auto result = create_uploadbuffer(mapped_buffer, configurable_properties::frame_count * perframe_buffersize);
    update_perframebuffer(*mapped_buffer, data_start, perframe_buffersize);

    return result;
}

D3D12_GPU_VIRTUAL_ADDRESS gfx::get_perframe_gpuaddress(D3D12_GPU_VIRTUAL_ADDRESS start, std::size_t perframe_buffersize)
{
    auto frame_idx = game_engine::g_engine->get_frame_index();
    return start + perframe_buffersize * frame_idx;
}

void gfx::update_perframebuffer(uint8_t* mapped_buffer, void* data_start, std::size_t const perframe_buffersize)
{
    auto frame_idx = game_engine::g_engine->get_frame_index();
    memcpy(mapped_buffer + perframe_buffersize * frame_idx, data_start, perframe_buffersize);
}

ComPtr<ID3D12Resource> gfx::create_uploadbuffer(uint8_t** mapped_buffer, std::size_t const buffersize)
{
    auto device = game_engine::g_engine->get_device();

    ComPtr<ID3D12Resource> vb_upload;
    if (buffersize > 0)
    {
        auto resource_desc = CD3DX12_RESOURCE_DESC::Buffer(buffersize);
        auto heap_props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        ThrowIfFailed(device->CreateCommittedResource(&heap_props, D3D12_HEAP_FLAG_NONE, &resource_desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(vb_upload.GetAddressOf())));

        // We do not intend to read from this resource on the CPU.
        ThrowIfFailed(vb_upload->Map(0, nullptr, reinterpret_cast<void**>(mapped_buffer)));
    }

    return vb_upload;
}