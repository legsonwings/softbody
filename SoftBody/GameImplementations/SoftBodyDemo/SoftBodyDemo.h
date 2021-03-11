#pragma once

#include "GameBase.h"
#include "Engine/SimpleMath.h"
#include "Engine/SimpleCamera.h"

// TODO : Refactor this file
#include "Engine/Shapes.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

class game_engine;

namespace gfx
{
	struct instance_data;

	template<typename body_type>
	class body_static;

	template<typename body_type>
	class body_dynamic;
}

class soft_body final : public game_base
{
public:
	soft_body(game_engine const* engine);

	void update(float dt) override;
	void render(float dt) override;
	void populate_command_list() override;
	
	std::vector<std::weak_ptr<gfx::body>> load_assets_and_geometry() override;

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

	struct controlnet_instance_data
	{
		XMFLOAT3 position;
	};

	ComPtr<ID3D12Resource> create_upload_buffer(uint8_t **mapped_buffer, size_t const buffer_size ) const;

	static constexpr unsigned c_num_controlpoints = 27;

	static constexpr const wchar_t* c_lines_meshshader_filename = L"LinesMS.cso";
	static constexpr const wchar_t* c_lines_pixelshader_filename = L"BasicPS.cso";
	static constexpr unsigned cb_alignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;

	SimpleCamera m_camera;

	ComPtr<ID3D12RootSignature> m_rootsignature;
	ComPtr<ID3D12RootSignature> m_rootsignature_lines;
	ComPtr<ID3D12PipelineState> m_pipelinestate;
	ComPtr<ID3D12PipelineState> m_pipelinestate_wireframe;
	ComPtr<ID3D12PipelineState> m_pipelinestate_lines;

	ComPtr<ID3D12Resource> m_constantbuffer;
	ComPtr<ID3D12Resource> m_vertexbuffer_controlnet;
	ComPtr<ID3D12Resource> m_uploadbuffer_controlnet;
	ComPtr<ID3D12Resource> m_instance_buffer_controlnet;

	uint8_t* m_ibv_mapped_controlnet = nullptr;
	uint8_t* m_cbv_databegin = nullptr;

	uint8_t constant_buffer_memory[sizeof(SceneConstantBuffer) + cb_alignment] = {};
	SceneConstantBuffer *m_constantbuffer_data = nullptr;;
	std::unique_ptr<controlnet_instance_data[]> m_cpu_instance_data_control_net;

	// TODO : We don't need this vector after they have been uploaded to GPU
	// Update setup to keep this alive only until then
	bool m_wireframe_toggle = false;
	std::vector<Geometry::Vector3> m_vertices_controlnet;

	std::shared_ptr<gfx::body_static<Geometry::circle>> circle;
	std::vector<std::shared_ptr<gfx::body_dynamic<Geometry::ffd_object>>> spheres;
};