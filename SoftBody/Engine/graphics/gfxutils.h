#pragma once

#include "gfxcore.h"
#include <unordered_map>

using namespace DirectX;
using Microsoft::WRL::ComPtr;

namespace gfx
{
	using psomapref = std::unordered_map<std::string, pipeline_objects> const&;
	using maetrialmapref = std::unordered_map<std::string, material> const&;
	//using materialref = maetrialmapref::
	psomapref getpsomap();
	maetrialmapref getmatmap();
	material const& getmat(std::string const &name);
	void init_pipelineobjects();
	void deinit_pipelineobjects();
	ComPtr<ID3D12Resource> create_uploadbuffer(uint8_t** mapped_buffer, std::size_t const buffersize);
	void addpso(std::string const& name, std::wstring const& as, std::wstring const& ms, std::wstring const& ps, psoflags const &flags = psoflags::none);
}