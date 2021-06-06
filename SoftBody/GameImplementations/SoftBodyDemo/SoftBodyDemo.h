#pragma once

#include "GameBase.h"
#include "Engine/SimpleMath.h"
#include "Engine/SimpleCamera.h"

#include "Engine/geometry/Shapes.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

class game_engine;

namespace gfx
{
	struct instance_data;

	template<typename geometry_t, gfx::topology primitive_t = gfx::topology::triangle>
	class body_static;

	template<typename geometry_t, gfx::topology primitive_t = gfx::topology::triangle>
	class body_dynamic;
}

class soft_body final : public game_base
{
public:
	soft_body(game_engine const* engine);

	void update(float dt) override;
	void render(float dt) override;
	
	std::vector<ComPtr<ID3D12Resource>> load_assets_and_geometry() override;

	void switch_cameraview();
	void on_key_down(unsigned key) override;
	void on_key_up(unsigned key) override;

private:

	_declspec(align(256u)) struct SceneConstantBuffer
	{
		XMFLOAT3 CamPos;
		uint8_t padding0[4];
		XMFLOAT4X4 World;
		XMFLOAT4X4 WorldView;
		XMFLOAT4X4 WorldViewProj;
	};

	static constexpr unsigned cb_alignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;

	SimpleCamera m_camera;

	ComPtr<ID3D12RootSignature> m_rootsignature;
	ComPtr<ID3D12RootSignature> m_rootsignature_lines;
	ComPtr<ID3D12PipelineState> m_pipelinestate;
	ComPtr<ID3D12PipelineState> m_pipelinestate_wireframe;
	ComPtr<ID3D12PipelineState> m_pipelinestate_lines;
	ComPtr<ID3D12Resource>		m_constantbuffer;

	uint8_t* m_cbv_databegin = nullptr;
	uint8_t constant_buffer_memory[sizeof(SceneConstantBuffer) + cb_alignment] = {};
	SceneConstantBuffer *m_constantbuffer_data = nullptr;

	bool m_wireframe_toggle = false;
	bool m_debugviz_toggle = true;

	uint8_t camera_view = 0;
	bool toggle1 = false;
	bool toggle2 = false;

	std::vector<gfx::body_dynamic<geometry::ffd_object>> dynamicbodies_tri;
	std::vector<gfx::body_dynamic<geometry::ffd_object const&, gfx::topology::line>> dynamicbodies_line;
	std::vector<gfx::body_static<geometry::ffd_object const&, gfx::topology::line>> staticbodies_lines;

	ComPtr<ID3D12Resource> create_upload_buffer(uint8_t** mapped_buffer, size_t const buffer_size) const;
	
	template<typename geometry_t, gfx::topology primitive_t = gfx::topology::triangle>
	typename std::enable_if_t<primitive_t == gfx::topology::triangle, void> dispatch_bodies(std::vector<gfx::body_dynamic<geometry_t>>&);

	template<typename geometry_t, gfx::topology primitive_t>
	typename std::enable_if_t<primitive_t == gfx::topology::line, void> dispatch_bodies(std::vector<gfx::body_dynamic<geometry_t, primitive_t>>&);

	template<typename geometry_t, gfx::topology primitive_t = gfx::topology::triangle>
	typename std::enable_if_t<primitive_t == gfx::topology::triangle, void> dispatch_bodies(std::vector<gfx::body_static<geometry_t>>&);

	template<typename geometry_t, gfx::topology primitive_t>
	typename std::enable_if_t<primitive_t == gfx::topology::line, void> dispatch_bodies(std::vector<gfx::body_static<geometry_t, primitive_t>>&);
};