#pragma once

#include "Engine/engineutils.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

constexpr unsigned cb_alignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;

struct cb_memory
{
	static constexpr uint buffermemory_size = 4096;
	uint8_t constant_buffer_memory[buffermemory_size];
};

struct constant_buffer
{
private:
	uint cb_size = 0;
	cb_memory buffermemory;
	uint8_t *cbdata_start = nullptr;
	uint8_t* cbupload_start = nullptr;
	ComPtr<ID3D12Resource> cb_resource;
	void createresources(uint _cb_size);
	void set_data(uint8_t const* data_start);
public:
	constant_buffer() = default;

	template<typename data_t>
	void createresources();

	template<typename data_t>
	void set_data(data_t const *_cbdata);

	D3D12_GPU_VIRTUAL_ADDRESS get_gpuaddress() const;
};

template<typename data_t>
inline void constant_buffer::createresources()
{
	createresources(sizeof(data_t));
}

template<typename data_t>
inline void constant_buffer::set_data(data_t const *_cbdata)
{
	assert(cbdata_start);
	assert(sizeof(data_t) == cb_size);
	assert(cb_size + cb_alignment <= cb_memory::buffermemory_size);
	set_data(reinterpret_cast<uint8_t const*>(_cbdata));
}