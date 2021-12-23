#include "gfxmemory.h"

//void constant_buffer::createresources(uint _cb_size)
//{
//	cb_size = _cb_size;
//	cb_resource = gfx::create_uploadbuffer(&cbupload_start, cb_size);
//
//	uintptr_t const temp = reinterpret_cast<uintptr_t>(buffermemory.constant_buffer_memory) + cbalignment - 1;
//	cbdata_start = reinterpret_cast<uint8_t*>(temp & ~static_cast<uintptr_t>(cbalignment - 1));
//}
//
//void constant_buffer::set_data(uint8_t const* data_start)
//{
//	// const buffers need to be aligned
//	// copy the buffer to aligned memory first
//	memcpy(cbdata_start, data_start, cb_size);
//	memcpy(cbupload_start + cb_size * gfx::globalresources::get().frameindex(), cbdata_start, cb_size);
//}
//
//D3D12_GPU_VIRTUAL_ADDRESS constant_buffer::get_gpuaddress() const
//{
//	return cb_resource->GetGPUVirtualAddress() + gfx::globalresources::get().frameindex() * cb_size;
//}

alignedlinearallocator::alignedlinearallocator(uint alignment) : _alignment(alignment)
{
	assert(ispowtwo(_alignment));
	uintptr_t const temp = reinterpret_cast<uintptr_t>(_buffer) + _alignment - 1;
	_currentpos = reinterpret_cast<std::byte*>(temp & ~static_cast<uintptr_t>(_alignment - 1));
}

bool alignedlinearallocator::canallocate(uint size) const { return ((_currentpos - &_buffer[0]) + size) < buffersize; }
