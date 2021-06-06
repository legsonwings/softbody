#pragma once

#include <vector>
#include <utility>

using namespace DirectX;
using Microsoft::WRL::ComPtr;

namespace gfx
{
	struct pipeline_objects
	{
		ComPtr<ID3D12PipelineState> pso;
		ComPtr<ID3D12PipelineState> pso_wireframe;
		ComPtr<ID3D12RootSignature> root_signature;
	};

	class body
	{
	public:
		virtual void update(float dt) {};
		virtual std::vector<ComPtr<ID3D12Resource>> create_resources() { return {}; };
		virtual pipeline_objects const& get_pipelineobjects() const = 0;
		virtual D3D12_GPU_VIRTUAL_ADDRESS get_vertexbuffer_gpuaddress() const = 0;
	};
}