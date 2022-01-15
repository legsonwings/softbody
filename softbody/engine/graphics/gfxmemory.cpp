#include "stdx/stdx.h"
#include "gfxmemory.h"

#include <memory>

alignedlinearallocator::alignedlinearallocator(uint alignment) : _alignment(alignment)
{
	std::size_t sz = buffersize;
	assert(stdx::ispowtwo(_alignment));
	void* ptr = _buffer;
	_currentpos = reinterpret_cast<std::byte*>(std::align(_alignment, buffersize - alignment, ptr, sz));
}

bool alignedlinearallocator::canallocate(uint size) const { return ((_currentpos - &_buffer[0]) + size) < buffersize; }
