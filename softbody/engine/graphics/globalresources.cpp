#include "globalresources.h"
#include "gfxutils.h"

#include "engine/dxsamplehelper.h"

#define NOMINMAX
#include <assert.h>

void gfx::globalresources::init() 
{
    addpso("lines", L"default_as.cso", L"lines_ms.cso", L"basic_ps.cso");
    addpso("default", L"default_as.cso", L"default_ms.cso", L"default_ps.cso");
    addpso("texturess", L"", L"texturess_ms.cso", L"texturess_ps.cso");
    addpso("instancedlines", L"default_as.cso", L"linesinstances_ms.cso", L"basic_ps.cso");
    addpso("instanced", L"default_as.cso", L"instances_ms.cso", L"instances_ps.cso");
    addpso("transparent", L"default_as.cso", L"default_ms.cso", L"default_ps.cso", psoflags::transparent);
    addpso("transparent_twosided", L"default_as.cso", L"default_ms.cso", L"default_ps.cso", psoflags::transparent | psoflags::twosided);
    addpso("wireframe", L"default_as.cso", L"default_ms.cso", L"default_ps.cso", psoflags::wireframe | psoflags::transparent);
    addpso("instancedtransparent", L"default_as.cso", L"instances_ms.cso", L"instances_ps.cso", psoflags::transparent);

    addmat("black", material().diffuse(gfx::color::black));
    addmat("white", material().diffuse(gfx::color::white));
    addmat("red", material().diffuse(gfx::color::red));

    WCHAR assetsPath[512];
    GetAssetsPath(assetsPath, _countof(assetsPath));
    _assetspath = assetsPath;

    D3D12_DESCRIPTOR_HEAP_DESC srvheapdesc = {};
    srvheapdesc.NumDescriptors = 1;
    srvheapdesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvheapdesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    _srvheap = createsrvdescriptorheap(srvheapdesc);
}

gfx::viewinfo& gfx::globalresources::view() { return _view; }
gfx::sceneconstants& gfx::globalresources::globals() { return _constbufferdata; }
gfx::psomapref gfx::globalresources::psomap() const { return _psos; }
gfx::matmapref gfx::globalresources::matmap() const { return _materials; }
ComPtr<ID3D12DescriptorHeap>& gfx::globalresources::srvheap() { return _srvheap; }
ComPtr<ID3D12Device2>& gfx::globalresources::device() { return _device; }
ComPtr<ID3D12GraphicsCommandList6>& gfx::globalresources::cmdlist() { return _commandlist; }
void gfx::globalresources::frameindex(uint idx) { _frameindex = idx; }
uint gfx::globalresources::frameindex() const { return _frameindex; }
std::wstring gfx::globalresources::assetfullpath(std::wstring const& path) const { return _assetspath + path; }
void gfx::globalresources::psodesc(D3DX12_MESH_SHADER_PIPELINE_STATE_DESC const& psodesc) { _psodesc = psodesc; }
gfx::materialcref gfx::globalresources::addmat(std::string const& name, material const& mat, bool twosided) { return _materials.insert({ name, {mat, twosided} }).first->second; }

uint gfx::globalresources::dxgisize(DXGI_FORMAT format) 
{ 
    return _dxgisizes.contains(format) ? _dxgisizes[format] : stdx::invalid<uint>(); 
}

gfx::materialcref gfx::globalresources::mat(std::string const& name)
{
    if (auto const& found = _materials.find(name); found != _materials.end())
        return found->second;

    return _defaultmat;
}

void gfx::globalresources::addpso(std::string const& name, std::wstring const& as, std::wstring const& ms, std::wstring const& ps, uint flags)
{
    if (_psos.find(name) != _psos.cend())
    {
        assert("trying to add pso of speciifed name when it already exists");
    }

    shader ampshader, meshshader, pixelshader;

    if (!as.empty())
        ReadDataFromFile(assetfullpath(as).c_str(), &ampshader.data, &ampshader.size);

    ReadDataFromFile(assetfullpath(ms).c_str(), &meshshader.data, &meshshader.size);
    ReadDataFromFile(assetfullpath(ps).c_str(), &pixelshader.data, &pixelshader.size);

    // pull root signature from the precompiled mesh shaders.
    // todo : root signatures will be duplicated in case shaders share root sig or same shader is used to create multiple psos
    ThrowIfFailed(_device->CreateRootSignature(0, meshshader.data, meshshader.size, IID_PPV_ARGS(_psos[name].root_signature.GetAddressOf())));

    D3DX12_MESH_SHADER_PIPELINE_STATE_DESC pso_desc = _psodesc;

    pso_desc.pRootSignature = _psos[name].root_signature.Get();

    if (!as.empty())
        pso_desc.AS = { ampshader.data, ampshader.size };

    pso_desc.MS = { meshshader.data, meshshader.size };
    pso_desc.PS = { pixelshader.data, pixelshader.size };

    if (flags & psoflags::transparent)
    {
        D3D12_RENDER_TARGET_BLEND_DESC transparency_blenddesc = CD3DX12_RENDER_TARGET_BLEND_DESC(D3D12_DEFAULT);
        transparency_blenddesc.BlendEnable = true;
        transparency_blenddesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
        transparency_blenddesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;

        pso_desc.BlendState.RenderTarget[0] = transparency_blenddesc;
    }

    auto psostream = CD3DX12_PIPELINE_MESH_STATE_STREAM(pso_desc);

    D3D12_PIPELINE_STATE_STREAM_DESC stream_desc;
    stream_desc.pPipelineStateSubobjectStream = &psostream;
    stream_desc.SizeInBytes = sizeof(psostream);

    ThrowIfFailed(_device->CreatePipelineState(&stream_desc, IID_PPV_ARGS(_psos[name].pso.GetAddressOf())));

    if (flags & psoflags::wireframe)
    {
        D3DX12_MESH_SHADER_PIPELINE_STATE_DESC wireframepso = pso_desc;
        wireframepso.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;

        auto psostream = CD3DX12_PIPELINE_MESH_STATE_STREAM(wireframepso);
        D3D12_PIPELINE_STATE_STREAM_DESC stream_desc;
        stream_desc.pPipelineStateSubobjectStream = &psostream;
        stream_desc.SizeInBytes = sizeof(psostream);
        ThrowIfFailed(_device->CreatePipelineState(&stream_desc, IID_PPV_ARGS(_psos[name].pso_wireframe.GetAddressOf())));
    }

    if (flags & psoflags::twosided)
    {
        pso_desc.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT;
        auto psostream_twosides = CD3DX12_PIPELINE_MESH_STATE_STREAM(pso_desc);

        D3D12_PIPELINE_STATE_STREAM_DESC stream_desc_twosides;
        stream_desc_twosides.pPipelineStateSubobjectStream = &psostream_twosides;
        stream_desc_twosides.SizeInBytes = sizeof(psostream_twosides);

        ThrowIfFailed(_device->CreatePipelineState(&stream_desc_twosides, IID_PPV_ARGS(_psos[name].pso_twosided.GetAddressOf())));
    }
}