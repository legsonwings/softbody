#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <string>
#include <array>
#include <vector>
#include <utility>

#include "engine/core.h"
#include "engine/stdxcore.h"

struct renderparams;

namespace gfx
{
	struct bodyparams
	{
		std::string psoname;
		std::string matname;
		std::array<uint, 2> texdims;
	};

	class bodyinterface
	{
		bodyparams params;
	public:
		bodyinterface(bodyparams const& _bodyparams) : params(_bodyparams) {}

		bodyparams const& getparams() const { return params;  }
		virtual void update(float dt) {};
		virtual void render(float dt, renderparams const &) {};
		virtual std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> create_resources() { return {}; };
		virtual D3D12_GPU_VIRTUAL_ADDRESS get_vertexbuffer_gpuaddress() const = 0;
	};
}