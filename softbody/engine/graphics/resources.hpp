#pragma once

#include "stdx/stdx.h"
#include "stdx/vec.h"
#include "gfxmemory.h"
#include "gfxutils.h"

#include <wrl.h>
#include "d3dx12.h"

#define NOMINMAX
#include <assert.h>

#include <cstdint>

namespace gfx
{
	using Microsoft::WRL::ComPtr;

	struct cballocationhelper
	{
		static constexpr unsigned cbalignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
		static alignedlinearallocator& allocator() { static alignedlinearallocator allocator{ cbalignment }; return allocator; }
	};

	template<typename t>
	requires (sizeof(t) % 256 == 0)
	struct constantbuffer
	{
		void createresource() 
		{
			_data = cballocationhelper::allocator().allocate<t>();
			_buffer = create_uploadbuffer(&_mappeddata, size());
		}

		t& data() const { return *_data; }
		void updateresource() { update_perframebuffer(_mappeddata, _data, size()); }

		template<typename u>
		requires std::same_as<t, std::decay_t<u>>
		void updateresource(u&& data) 
		{
			*_data = data;
			update_perframebuffer(_mappeddata, _data, size()); 
		}

		uint size() const { return sizeof(t); }
		D3D12_GPU_VIRTUAL_ADDRESS gpuaddress() const { return get_perframe_gpuaddress(_buffer->GetGPUVirtualAddress(), size()); }

		t* _data;
		uint8_t* _mappeddata = nullptr;
		ComPtr<ID3D12Resource> _buffer;
	};

	template<typename t>
	struct staticbuffer
	{
		ComPtr<ID3D12Resource> createresources(std::vector<t> const& data)
		{
			_count = data.size();
			auto const bufferresources = create_defaultbuffer(data.data(), size());
			_buffer = bufferresources.first;
			return bufferresources.second;
		}

		uint count() const { return _count; }
		uint size() const { return _count * sizeof(t); }
		D3D12_GPU_VIRTUAL_ADDRESS gpuaddress() const { return _buffer->GetGPUVirtualAddress(); }

		uint _count = 0;
		ComPtr<ID3D12Resource> _buffer;
	};

	template<typename t>
	struct dynamicbuffer
	{
		void createresource(std::vector<t> const& data)
		{
			_count = data.size();
			_buffer = create_uploadbuffer(&_mappeddata, size());
			updateresource(data);
		}

		uint count() const { return _count; }
		uint size() const { return count() * sizeof(t); }
		void updateresource(std::vector<t> const& data) { update_perframebuffer(_mappeddata, data.data(), size()); }
		D3D12_GPU_VIRTUAL_ADDRESS gpuaddress() const { return get_perframe_gpuaddress(_buffer->GetGPUVirtualAddress(), size()); }

		uint _count = 0;
		uint8_t* _mappeddata = nullptr;
		ComPtr<ID3D12Resource> _buffer;
	};

	struct texture
	{
		void createresource(uint heapidx, std::vector<uint8_t> const& texturedata, ID3D12DescriptorHeap *srvheap)
		{
			_heapidx = heapidx;
			_srvheap = srvheap;
			_bufferupload = create_uploadbufferunmapped(size());
			_texture = createtexture_default(_dims[0], _dims[1], _format);

			D3D12_SHADER_RESOURCE_VIEW_DESC srvdesc = {};
			srvdesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvdesc.Format = _format;
			srvdesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvdesc.Texture2D.MipLevels = 1;

			createsrv(srvdesc, _texture.Get(), _srvheap.Get(), _heapidx);
			updateresource(texturedata);
		}

		void updateresource(std::vector<uint8_t> const& texturedata)
		{
			D3D12_SUBRESOURCE_DATA subresdata;
			subresdata.pData = texturedata.data();
			subresdata.RowPitch = _dims[0] * 4;
			subresdata.SlicePitch = _dims[0] * _dims[1] * 4;

			updatesubres(_texture.Get(), _bufferupload.Get(), &subresdata);
		}
		
		D3D12_GPU_DESCRIPTOR_HANDLE deschandle() const
		{
			return CD3DX12_GPU_DESCRIPTOR_HANDLE(_srvheap->GetGPUDescriptorHandleForHeapStart(), static_cast<INT>(_heapidx * srvsbvuav_descincrementsize()));
		}

		uint size() const { return _dims[0] * _dims[1] * dxgiformatsize(_format); }

		uint _heapidx;
		DXGI_FORMAT _format;
		stdx::vecui2 _dims;
		ComPtr<ID3D12Resource> _texture;
		ComPtr<ID3D12Resource> _bufferupload;
		ComPtr<ID3D12DescriptorHeap> _srvheap;
	};
}