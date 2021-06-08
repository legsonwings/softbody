#include "stdafx.h"
#include "gfxutils.h"
#include "Engine/DXSampleHelper.h"
#include "Engine/interfaces/engineinterface.h"

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