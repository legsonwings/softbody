#pragma once

#include <vector>
#include <utility>

using namespace DirectX;
using Microsoft::WRL::ComPtr;

struct renderparams;

namespace gfx
{
	struct bodyparams
	{
		std::string psoname;
	};

	class bodyinterface
	{
		bodyparams params;
	public:
		bodyinterface(bodyparams const& _bodyparams) : params(_bodyparams) {}

		bodyparams const& getparams() const { return params;  }
		virtual void update(float dt) {};
		virtual void render(float dt, renderparams const &) {};
		virtual std::vector<ComPtr<ID3D12Resource>> create_resources() { return {}; };
		virtual D3D12_GPU_VIRTUAL_ADDRESS get_vertexbuffer_gpuaddress() const = 0;
	};
}