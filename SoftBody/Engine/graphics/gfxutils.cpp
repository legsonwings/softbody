#include "stdafx.h"
#include "gfxutils.h"
#include "Engine/DXSampleHelper.h"
#include "Engine/interfaces/engineinterface.h"

namespace gfx
{
    std::unordered_map<std::string, pipeline_objects> psos;
}

gfx::psomapref gfx::getpsomap()
{
    return psos;
}

void gfx::init_pipelineobjects()
{
    addpso("lines", L"", L"linesMS.cso", L"BasicPS.cso");
    addpso("instancedlines", L"", L"linesinstancesMS.cso", L"BasicPS.cso");
    addpso("default", L"DefaultAS.cso", L"DefaultMS.cso", L"BasicLightingPS.cso");
    addpso("transparent", L"DefaultAS.cso", L"DefaultMS.cso", L"BasicLightingPS.cso", psoflags::transparent);
    addpso("instanced", L"InstancesAS.cso", L"InstancesMS.cso", L"BasicLightingPS.cso");
    addpso("wireframe", L"InstancesAS.cso", L"InstancesMS.cso", L"BasicLightingPS.cso", psoflags::wireframe);
    addpso("instancedtransparent", L"InstancesAS.cso", L"InstancesMS.cso", L"BasicLightingPS.cso", psoflags::transparent);
}

void gfx::deinit_pipelineobjects()
{
    psos.clear();
}

ComPtr<ID3D12Resource> gfx::create_uploadbuffer(uint8_t** mapped_buffer, std::size_t const buffersize)
{
    auto device = game_engine::g_engine->get_device();

    ComPtr<ID3D12Resource> vb_upload;
    if (buffersize > 0)
    {
        auto resource_desc = CD3DX12_RESOURCE_DESC::Buffer(buffersize);
        auto heap_props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        ThrowIfFailed(device->CreateCommittedResource(&heap_props, D3D12_HEAP_FLAG_NONE, &resource_desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(vb_upload.GetAddressOf())));

        // We do not intend to read from this resource on the CPU.
        ThrowIfFailed(vb_upload->Map(0, nullptr, reinterpret_cast<void**>(mapped_buffer)));
    }

    return vb_upload;
}

void gfx::addpso(std::string const& name, std::wstring const& as, std::wstring const& ms, std::wstring const& ps, psoflags const& flags)
{    
    if (psos.find(name) != psos.cend())
    {
        assert("trying to add pso of speciifed name when it already exists");
    }

    auto engine = game_engine::g_engine;
    auto device = engine->get_device();
    shader ampshader, meshshader, pixelshader;

    // todo : cache file reads
    if (!as.empty())
        ReadDataFromFile(engine->get_asset_fullpath(as).c_str(), &ampshader.data, &ampshader.size);

    ReadDataFromFile(engine->get_asset_fullpath(ms).c_str(), &meshshader.data, &meshshader.size);
    ReadDataFromFile(engine->get_asset_fullpath(ps).c_str(), &pixelshader.data, &pixelshader.size);

    // pull root signature from the precompiled mesh shaders.
    // todo : root signatures will contain duplicate entries in case shaders share root sig or same shader is used to create multiple psos
    ThrowIfFailed(device->CreateRootSignature(0, meshshader.data, meshshader.size, IID_PPV_ARGS(psos[name].root_signature.GetAddressOf())));

    D3DX12_MESH_SHADER_PIPELINE_STATE_DESC pso_desc = engine->get_pso_desc();

    pso_desc.pRootSignature = psos[name].root_signature.Get();

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
        pso_desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    }

    if (flags & psoflags::wireframe)
    {
        D3DX12_MESH_SHADER_PIPELINE_STATE_DESC wireframepso = pso_desc;
        pso_desc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
    }

    auto psostream = CD3DX12_PIPELINE_MESH_STATE_STREAM(pso_desc);

    D3D12_PIPELINE_STATE_STREAM_DESC stream_desc;
    stream_desc.pPipelineStateSubobjectStream = &psostream;
    stream_desc.SizeInBytes = sizeof(psostream);

    ThrowIfFailed(device->CreatePipelineState(&stream_desc, IID_PPV_ARGS(psos[name].pso.GetAddressOf())));
}