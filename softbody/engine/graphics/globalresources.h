#pragma once

#include "stdx/stdx.h"
#include "gfxcore.h"

#include <wrl.h>
#include "d3dx12.h"

#include <string>
#include <unordered_map>

using namespace DirectX;
using Microsoft::WRL::ComPtr;

namespace gfx
{
	using psomapref = std::unordered_map<std::string, pipeline_objects> const&;
	using matmapref = std::unordered_map<std::string, stdx::ext<material, bool>> const&;
	using materialentry = stdx::ext<material, bool>;
	using materialcref = stdx::ext<material, bool> const&;

class globalresources
{
	viewinfo _view;
	uint _frameindex{0};
	std::wstring _assetspath;
	sceneconstants _constbufferdata;
	ComPtr<ID3D12DescriptorHeap> _srvheap;
	ComPtr<ID3D12Device2> _device;
	ComPtr<ID3D12GraphicsCommandList6> _commandlist;
	std::unordered_map<std::string, pipeline_objects> _psos;
	std::unordered_map<std::string, stdx::ext<material, bool>> _materials;
	stdx::ext<material, bool> _defaultmat{ {}, false };
	std::unordered_map<DXGI_FORMAT, uint> _dxgisizes{ {DXGI_FORMAT_R8G8B8A8_UNORM, 4} };
	D3DX12_MESH_SHADER_PIPELINE_STATE_DESC _psodesc;

	std::wstring assetfullpath(std::wstring const& path) const;
public:
	void init();

	viewinfo& view();
	sceneconstants& globals();
	psomapref psomap() const;
	matmapref matmap() const;
	ComPtr<ID3D12Device2>& device();
	ComPtr<ID3D12DescriptorHeap>& srvheap();
	ComPtr<ID3D12GraphicsCommandList6>& cmdlist();
	void frameindex(uint idx);
	uint frameindex() const;
	uint dxgisize(DXGI_FORMAT format);
	materialcref mat(std::string const& name);
	void psodesc(D3DX12_MESH_SHADER_PIPELINE_STATE_DESC const& psodesc);
	materialcref addmat(std::string const& name, material const& mat, bool twosided = false);
	void addpso(std::string const& name, std::wstring const& as, std::wstring const& ms, std::wstring const& ps, uint flags = psoflags::none);

	static globalresources& get() { static globalresources res; return res; }
};



}
