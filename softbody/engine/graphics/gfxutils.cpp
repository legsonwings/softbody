#include "gfxutils.h"
#include "globalresources.h"

#include "engine/engineutils.h"
#include "engine/dxhelpers.h"

std::string gfx::generaterandom_matcolor(stdx::ext<material, bool> definition, std::optional<std::string> const& preferred_name)
{
    static constexpr uint matgenlimit = 1000u;
    auto & re = engineutils::getrandomengine();
    static std::uniform_int_distribution<uint> matnumberdist(0, matgenlimit);
    static std::uniform_real_distribution<float> colordist(0.f, 1.f);

    vector3 const color = { colordist(re), colordist(re), colordist(re) };
    static const std::string basename("mat");

    std::string matname = preferred_name.has_value() ? preferred_name.value() : basename + std::to_string(matnumberdist(re));

    while (globalresources::get().matmap().find(matname) != globalresources::get().matmap().end()) { matname = basename + std::to_string(matnumberdist(re)); }

    definition->a = vector4(color.x, color.y, color.z, definition->a.w);
    globalresources::get().addmat(matname, definition, definition.ex());
    return matname;
}

void gfx::update_allframebuffers(std::byte* mapped_buffer, void const* data_start, uint const perframe_buffersize)
{
    for (uint i = 0; i < configurable_properties::frame_count; ++i)
        memcpy(mapped_buffer + perframe_buffersize * i, data_start, perframe_buffersize);
}

uint gfx::srvsbvuav_descincrementsize()
{
    auto device = globalresources::get().device();
    return static_cast<uint>(device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
}

uint gfx::dxgiformatsize(DXGI_FORMAT format)
{
    return globalresources::get().dxgisize(format);
}

ComPtr<ID3D12Resource> gfx::create_uploadbuffer(std::byte** mapped_buffer, uint const buffersize)
{
    auto device = globalresources::get().device();

    ComPtr<ID3D12Resource> b_upload;
    if (buffersize > 0)
    {
        auto resource_desc = CD3DX12_RESOURCE_DESC::Buffer(configurable_properties::frame_count * buffersize);
        auto heap_props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        ThrowIfFailed(device->CreateCommittedResource(&heap_props, D3D12_HEAP_FLAG_NONE, &resource_desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(b_upload.GetAddressOf())));

        // We do not intend to read from this resource on the CPU.
        ThrowIfFailed(b_upload->Map(0, nullptr, reinterpret_cast<void**>(mapped_buffer)));
    }

    return b_upload;
}

ComPtr<ID3D12Resource> gfx::create_uploadbufferunmapped(uint const buffersize)
{
    auto device = globalresources::get().device();

    ComPtr<ID3D12Resource> b_upload;
    if (buffersize > 0)
    {
        auto resource_desc = CD3DX12_RESOURCE_DESC::Buffer(configurable_properties::frame_count * buffersize);
        auto heap_props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        ThrowIfFailed(device->CreateCommittedResource(&heap_props, D3D12_HEAP_FLAG_NONE, &resource_desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(b_upload.GetAddressOf())));
    }

    return b_upload;
}

ComPtr<ID3D12DescriptorHeap> gfx::createsrvdescriptorheap(D3D12_DESCRIPTOR_HEAP_DESC heapdesc)
{
    auto device = globalresources::get().device();

    ComPtr<ID3D12DescriptorHeap> heap;
    ThrowIfFailed(device->CreateDescriptorHeap(&heapdesc, IID_PPV_ARGS(heap.ReleaseAndGetAddressOf())));
    return heap;
}

void gfx::createsrv(D3D12_SHADER_RESOURCE_VIEW_DESC srvdesc, ID3D12Resource* resource, ID3D12DescriptorHeap* srvheap, uint heapslot)
{
    auto device = globalresources::get().device();

    CD3DX12_CPU_DESCRIPTOR_HANDLE deschandle(srvheap->GetCPUDescriptorHandleForHeapStart(), static_cast<INT>(heapslot * srvsbvuav_descincrementsize()));
    device->CreateShaderResourceView(resource, &srvdesc, deschandle);
}

gfx::default_and_upload_buffers gfx::create_defaultbuffer(void const* datastart, std::size_t const vb_size)
{
    auto device = globalresources::get().device();

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

            // copy vertex data to upload heap
            memcpy(vb_upload_start, datastart, vb_size);

            vb_upload->Unmap(0, nullptr);
        }

        auto resource_transition = CD3DX12_RESOURCE_BARRIER::Transition(vb.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

        auto cmdlist = globalresources::get().cmdlist();

        // copy vertex data from upload heap to default heap
        cmdlist->CopyResource(vb.Get(), vb_upload.Get());
        cmdlist->ResourceBarrier(1, &resource_transition);
    }

    return { vb, vb_upload };
}

ComPtr<ID3D12Resource> gfx::createtexture_default(uint width, uint height, DXGI_FORMAT format)
{
    auto device = globalresources::get().device();
    auto texdesc = CD3DX12_RESOURCE_DESC::Tex2D(format, static_cast<UINT64>(width), static_cast<UINT>(height));

    ComPtr<ID3D12Resource> texdefault;
    auto defaultheap_desc = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    ThrowIfFailed(device->CreateCommittedResource(&defaultheap_desc, D3D12_HEAP_FLAG_NONE, &texdesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(texdefault.ReleaseAndGetAddressOf())));
    return texdefault;
}

uint gfx::updatesubres(ID3D12Resource* dest, ID3D12Resource* upload, D3D12_SUBRESOURCE_DATA const* srcdata)
{
    auto cmdlist = globalresources::get().cmdlist();
    return UpdateSubresources(cmdlist.Get(), dest, upload, 0, 0, 1, srcdata);
}

D3D12_GPU_VIRTUAL_ADDRESS gfx::get_perframe_gpuaddress(D3D12_GPU_VIRTUAL_ADDRESS start, std::size_t perframe_buffersize)
{
    auto frame_idx = globalresources::get().frameindex();
    return start + perframe_buffersize * frame_idx;
}

void gfx::update_perframebuffer(std::byte* mapped_buffer, void const* data_start, std::size_t const perframe_buffersize)
{
    auto frame_idx = globalresources::get().frameindex();
    memcpy(mapped_buffer + perframe_buffersize * frame_idx, data_start, perframe_buffersize);
}