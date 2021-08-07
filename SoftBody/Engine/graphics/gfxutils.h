#pragma once

#include "gfxcore.h"
#include <engine/engineutils.h>

#include <optional>
#include <unordered_map>

using namespace DirectX;
using Microsoft::WRL::ComPtr;

namespace gfx
{
	using psomapref = std::unordered_map<std::string, pipeline_objects> const&;
	using materialentry = xstd::ext<material, bool>;
	using materialcref = xstd::ext<material, bool> const&;

	viewinfo& getview();
	sceneconstants& getglobals();
	psomapref getpsomap();
	materialcref getmat(std::string const &name);
	std::string const& generaterandom_matcolor(materialcref definition, std::optional<std::string> const& preferred_name = {});

	void init_pipelineobjects();
	void deinit_pipelineobjects();
	ComPtr<ID3D12Resource> create_uploadbuffer(uint8_t** mapped_buffer, std::size_t const buffersize);
	void addpso(std::string const& name, std::wstring const& as, std::wstring const& ms, std::wstring const& ps, uint flags = psoflags::none);
}