#pragma once

using namespace DirectX;
using Microsoft::WRL::ComPtr;

namespace gfx
{
	ComPtr<ID3D12Resource> create_uploadbuffer(uint8_t** mapped_buffer, std::size_t const buffersize);
}