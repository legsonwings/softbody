#pragma once

#include <string>
#include <random>
using namespace DirectX;

struct constant_buffer;
struct configurable_properties;

class game_engine
{
public:
	virtual ~game_engine() {};
	virtual unsigned get_frame_index() const = 0;
	virtual ID3D12Device2* get_device() const = 0;
	virtual ID3D12GraphicsCommandList6* get_command_list() const = 0;
	virtual configurable_properties const& get_config_properties() const = 0;
	virtual D3DX12_MESH_SHADER_PIPELINE_STATE_DESC get_pso_desc() const = 0;
	virtual std::wstring get_asset_fullpath(std::wstring const& asset_name) const { return L""; }

	static game_engine* g_engine;
};
