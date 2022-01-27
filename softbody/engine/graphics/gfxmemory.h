#pragma once

#include "stdx/stdx.h"

#include <wrl.h>
#include <d3d12.h>

#include <cassert>
#include <cstddef>
#include <concepts>

struct alignedlinearallocator
{
	alignedlinearallocator(uint alignment);

	template<typename t>
	requires std::default_initializable<t>
	t* allocate()
	{
		uint const alignedsize = stdx::nextpowoftwomultiple(sizeof(t), _alignment);
		
		assert(_currentpos);
		assert(canallocate(alignedsize));
		t* r = reinterpret_cast<t*>(_currentpos);
		assert(r);
		*r = t{};
		_currentpos += alignedsize;
		return r;
	}
private:

	bool canallocate(uint size) const;

	static constexpr uint buffersize = 51200;
	std::byte _buffer[buffersize];
	std::byte* _currentpos = &_buffer[0];
	uint _alignment;
};