#include "gfxmemory.h"
#include <memory>

alignedlinearallocator::alignedlinearallocator(uint alignment) : _alignment(alignment)
{
	std::size_t sz;
	assert(ispowtwo(_alignment));
	void* ptr = _buffer;
	_currentpos = reinterpret_cast<std::byte*>(std::align(_alignment, buffersize, ptr, sz));
}

bool alignedlinearallocator::canallocate(uint size) const { return ((_currentpos - &_buffer[0]) + size) < buffersize; }
