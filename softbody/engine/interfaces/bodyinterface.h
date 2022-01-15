#pragma once

#include "stdx/vec.h"
#include "engine/core.h"
#include "engine/graphics/gfxcore.h"

#include <string>
#include <array>
#include <vector>
#include <utility>

#include <d3d12.h>

#define NOMINMAX
#include <wrl.h>

struct renderparams;

namespace gfx
{
	struct bodyparams
	{
		std::string psoname;
		std::string matname;

		stdx::vecui2 dims;
	};

	class bodyinterface
	{
		bodyparams params;
	public:
		bodyinterface(bodyparams const& _bodyparams) : params(_bodyparams) {}

		bodyparams const& getparams() const { return params;  }
		virtual void update(float dt) {};
		virtual void render(float dt, renderparams const &) {};
		virtual gfx::resourcelist create_resources() { return {}; };
		virtual D3D12_GPU_VIRTUAL_ADDRESS get_vertexbuffer_gpuaddress() const { return 0; };
	};
}