#include "stdafx.h"
#include "body.h"
#include "gfxutils.h"
#include "engine/DXSampleHelper.h"
#include "engine/interfaces/engineinterface.h"

namespace gfx
{    
    void update_allframebuffers(uint8_t* mapped_buffer, void* data_start, std::size_t const perframe_buffersize, std::size_t framecount)
    {
        for (size_t i = 0; i < framecount; ++i)
        {
            memcpy(mapped_buffer + perframe_buffersize * i, data_start, perframe_buffersize);
        }
    }
}

void gfx::dispatch(resource_bindings const& bindings)
{
    auto engine = game_engine::g_engine;
    auto cmd_list = engine->get_command_list();

    cmd_list->SetPipelineState(bindings.pso.Get());
    cmd_list->SetGraphicsRootSignature(bindings.root_signature.Get());

    cmd_list->SetGraphicsRootConstantBufferView(bindings.constant.slot, bindings.constant.address);
    cmd_list->SetGraphicsRootShaderResourceView(bindings.vertex.slot, bindings.vertex.address);
    cmd_list->SetGraphicsRoot32BitConstants(bindings.rootconstants.slot, static_cast<UINT>(bindings.rootconstants.values.size() / 4), bindings.rootconstants.values.data(), 0);

    if (bindings.instance.address != 0)
        cmd_list->SetGraphicsRootShaderResourceView(bindings.instance.slot, bindings.instance.address);

    cmd_list->DispatchMesh(1, 1, 1);
}

void gfx::dispatch_lines(resource_bindings const& bindings, uint32_t numlines)
{
    auto engine = game_engine::g_engine;
    auto cmd_list = engine->get_command_list();

    cmd_list->SetPipelineState(bindings.pso.Get());
    cmd_list->SetGraphicsRootSignature(bindings.root_signature.Get());

    cmd_list->SetGraphicsRootConstantBufferView(bindings.constant.slot, bindings.constant.address);
    cmd_list->SetGraphicsRootShaderResourceView(bindings.vertex.slot, bindings.vertex.address);
    cmd_list->SetGraphicsRoot32BitConstants(bindings.rootconstants.slot, static_cast<UINT>(bindings.rootconstants.values.size() / 4), bindings.rootconstants.values.data(), 0);

    if(bindings.instance.address != 0)
        cmd_list->SetGraphicsRootShaderResourceView(bindings.instance.slot, bindings.instance.address);

    unsigned const num_ms = (numlines / MAX_LINES_PER_MS) + ((numlines % MAX_LINES_PER_MS) == 0 ? 0 : 1);

    unsigned num_lines_processed = 0;
    for (unsigned ms_idx = 0; ms_idx < num_ms; ++ms_idx)
    {
        cmd_list->SetGraphicsRoot32BitConstant(2, num_lines_processed, 0);
        cmd_list->DispatchMesh(1, 1, 1);

        num_lines_processed += MAX_LINES_PER_MS;
    }
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
    update_allframebuffers(*mapped_buffer, data_start, perframe_buffersize, configurable_properties::frame_count);

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