#pragma once

#include "engine/engineutils.h"
#include "engine/SimpleMath.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

struct ID3D12PipelineState;
struct ID3D12RootSignature;

namespace gfx
{
    enum class topology
    {
        triangle,
        line
    };

    enum psoflags
    {
        none = 0x0,
        wireframe = 0x1,
        transparent = 0x2,
        count
    };

    struct shader
    {
        byte* data;
        uint32_t size;
    };

    struct renderparams
    {
        bool wireframe = false;
        D3D12_GPU_VIRTUAL_ADDRESS cbaddress;
    };

    struct instance_data
    {
        DirectX::SimpleMath::Vector3 position = {};
        DirectX::SimpleMath::Vector3 color = { 1.f, 0.f, 0.f };
    };

    struct buffer
    {
        uint32_t slot;
        D3D12_GPU_VIRTUAL_ADDRESS address = 0;
    };

    struct rootconstants
    {
        uint32_t slot;
        std::vector<uint8_t> values;
    };

    struct resource_bindings
    {
        buffer constant;
        buffer vertex;
        buffer instance;
        rootconstants rootconstants;
        ComPtr<ID3D12PipelineState> pso;
        ComPtr<ID3D12RootSignature> root_signature;
    };

    struct pipeline_objects
    {
        ComPtr<ID3D12PipelineState> pso;
        ComPtr<ID3D12RootSignature> root_signature;
    };
}