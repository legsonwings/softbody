#include "gfxmemory.h"
#include "gfxutils.h"
#include "globalresources.h"

void constant_buffer::createresources(uint _cb_size)
{
	cb_size = _cb_size;
	cb_resource = gfx::create_uploadbuffer(&cbupload_start, cb_size);

	uintptr_t const temp = reinterpret_cast<uintptr_t>(buffermemory.constant_buffer_memory) + cb_alignment - 1;
	cbdata_start = reinterpret_cast<uint8_t*>(temp & ~static_cast<uintptr_t>(cb_alignment - 1));
}

void constant_buffer::set_data(uint8_t const* data_start)
{
	// const buffers need to be aligned
	// copy the buffer to aligned memory first
	memcpy(cbdata_start, data_start, cb_size);
	memcpy(cbupload_start + cb_size * gfx::globalresources::get().frameindex(), cbdata_start, cb_size);
}

D3D12_GPU_VIRTUAL_ADDRESS constant_buffer::get_gpuaddress() const
{
	return cb_resource->GetGPUVirtualAddress() + gfx::globalresources::get().frameindex() * cb_size;
}
