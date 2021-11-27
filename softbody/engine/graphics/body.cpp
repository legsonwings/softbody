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

ComPtr<ID3D12DescriptorHeap> gfx::createsrvdescriptorheap(D3D12_DESCRIPTOR_HEAP_DESC heapdesc)
{
    auto engine = game_engine::g_engine;
    auto device = engine->get_device();

    ComPtr<ID3D12DescriptorHeap> heap;
    ThrowIfFailed(device->CreateDescriptorHeap(&heapdesc, IID_PPV_ARGS(heap.ReleaseAndGetAddressOf())));
    return heap;
}

void gfx::createsrv(D3D12_SHADER_RESOURCE_VIEW_DESC srvdesc, ComPtr<ID3D12Resource> resource, ComPtr<ID3D12DescriptorHeap> srvheap)
{
    auto engine = game_engine::g_engine;
    auto device = engine->get_device();

    device->CreateShaderResourceView(resource.Get(), &srvdesc, srvheap->GetCPUDescriptorHandleForHeapStart());
}

void gfx::dispatch(resource_bindings const& bindings, bool wireframe, bool twosided, uint dispatchx)
{
    auto engine = game_engine::g_engine;
    auto cmd_list = engine->get_command_list();

    cmd_list->SetGraphicsRootSignature(bindings.pipelineobjs.root_signature.Get());

    cmd_list->SetGraphicsRootConstantBufferView(bindings.constant.slot, bindings.constant.address);
    if (bindings.objectconstant.address != 0)
        cmd_list->SetGraphicsRootConstantBufferView(bindings.objectconstant.slot, bindings.objectconstant.address);
    cmd_list->SetGraphicsRootShaderResourceView(bindings.vertex.slot, bindings.vertex.address);
    cmd_list->SetGraphicsRoot32BitConstants(bindings.rootconstants.slot, static_cast<UINT>(bindings.rootconstants.values.size() / 4), bindings.rootconstants.values.data(), 0);

    if (bindings.instance.address != 0)
        cmd_list->SetGraphicsRootShaderResourceView(bindings.instance.slot, bindings.instance.address);

    if (bindings.srvheap)
        cmd_list->SetDescriptorHeaps(1, bindings.srvheap.GetAddressOf());

    // todo : hack for now, address is never used so its not needed
    cmd_list->SetGraphicsRootDescriptorTable(bindings.texture.slot, bindings.srvheap->GetGPUDescriptorHandleForHeapStart());

    if (wireframe && bindings.pipelineobjs.pso_wireframe)
        cmd_list->SetPipelineState(bindings.pipelineobjs.pso_wireframe.Get());
    else if (twosided && bindings.pipelineobjs.pso_twosided)
        cmd_list->SetPipelineState(bindings.pipelineobjs.pso_twosided.Get());
    else
        cmd_list->SetPipelineState(bindings.pipelineobjs.pso.Get());

    cmd_list->DispatchMesh(static_cast<UINT>(dispatchx), 1, 1);
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

ComPtr<ID3D12Resource> gfx::createtexture_default(uint width, uint height)
{
    auto device = game_engine::g_engine->get_device();
    auto texdesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, static_cast<UINT64>(width), static_cast<UINT>(height));

    ComPtr<ID3D12Resource> texdefault;
    auto defaultheap_desc = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    ThrowIfFailed(device->CreateCommittedResource(&defaultheap_desc, D3D12_HEAP_FLAG_NONE, &texdesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(texdefault.ReleaseAndGetAddressOf())));
    return texdefault;
}

ComPtr<ID3D12Resource> gfx::createupdate_uploadbuffer(uint8_t** mapped_buffer, void* data_start, std::size_t const perframe_buffersize)
{
    auto result = create_uploadbuffer(mapped_buffer, configurable_properties::frame_count * perframe_buffersize);
    update_allframebuffers(*mapped_buffer, data_start, perframe_buffersize, configurable_properties::frame_count);

    return result;
}
uint gfx::updatesubres(ID3D12Resource* dest, ID3D12Resource* upload, const D3D12_SUBRESOURCE_DATA* srcdata)
{
    auto cmdlist = game_engine::g_engine->get_command_list();
    return UpdateSubresources(cmdlist, dest, upload, 0, 0, 1, srcdata);
}
//
//ComPtr<ID3D12Resource> gfx::createupdate_uploadtexture(uint width, uint height, uint8_t** mappedtex, void* data_start, std::size_t const perframe_texsize)
//{
//    auto device = game_engine::g_engine->get_device();
//    auto cmdlist = game_engine::g_engine->get_command_list();
//
//    ComPtr<ID3D12Resource> b_upload;
//    if (perframe_texsize > 0)
//    {
//        D3D12_BOX box;
//        box.left = box.top = 0;
//        box.right = static_cast<UINT>(width);
//        box.bottom = static_cast<UINT>(height);
//
//        auto resource_desc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R32G32B32_FLOAT, static_cast<UINT64>(width), static_cast<UINT>(height));
//        auto heap_props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
//        ThrowIfFailed(device->CreateCommittedResource(&heap_props, D3D12_HEAP_FLAG_NONE, &resource_desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(b_upload.GetAddressOf())));
//        b_upload->WriteToSubresource(0, &box, data_start, width * 12, width * height * 12);
//        
//        // We do not intend to read from this resource on the CPU.
//        //ThrowIfFailed(b_upload->Map(0, nullptr, reinterpret_cast<void**>(mappedtex)));
//    }
//
//    update_allframebuffers(*mappedtex, data_start, perframe_texsize, configurable_properties::frame_count);
//
//    return b_upload;
//}

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