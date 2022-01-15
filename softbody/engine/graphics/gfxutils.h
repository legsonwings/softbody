#pragma once

#include "stdx/stdx.h"
#include "gfxcore.h"

#include <string>
#include <cstddef>
#include <optional>

using namespace DirectX;
using Microsoft::WRL::ComPtr;

namespace gfx
{
	std::string generaterandom_matcolor(stdx::ext<material, bool> definition, std::optional<std::string> const& preferred_name = {});

	using default_and_upload_buffers = std::pair<ComPtr<ID3D12Resource>, ComPtr<ID3D12Resource>>;

	uint dxgiformatsize(DXGI_FORMAT format);
	uint srvsbvuav_descincrementsize();
	ComPtr<ID3D12Resource> create_uploadbuffer(std::byte** mapped_buffer, uint const buffersize);
	ComPtr<ID3D12Resource> create_uploadbufferunmapped(uint const buffersize);
	ComPtr<ID3D12DescriptorHeap> createsrvdescriptorheap(D3D12_DESCRIPTOR_HEAP_DESC heapdesc);
	void createsrv(D3D12_SHADER_RESOURCE_VIEW_DESC srvdesc, ID3D12Resource* resource, ID3D12DescriptorHeap* srvheap, uint heapslot = 0);
	default_and_upload_buffers create_defaultbuffer(void const* datastart, std::size_t const vb_size);
	ComPtr<ID3D12Resource> createtexture_default(uint width, uint height, DXGI_FORMAT format);
	uint updatesubres(ID3D12Resource* dest, ID3D12Resource* upload, D3D12_SUBRESOURCE_DATA const* srcdata);
	D3D12_GPU_VIRTUAL_ADDRESS get_perframe_gpuaddress(D3D12_GPU_VIRTUAL_ADDRESS start, UINT64 perframe_buffersize);
	void update_perframebuffer(std::byte* mapped_buffer, void const* data_start, std::size_t const perframe_buffersize);
	void update_allframebuffers(std::byte* mapped_buffer, void const* data_start, uint const perframe_buffersize);
}