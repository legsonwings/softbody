#pragma once

#include "engine/geometry/geodefines.h"
#include "engine/engineutils.h"
#include "engine/SimpleMath.h"

#include <string>

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
        // todo : this is not used now
        bool wireframe = false;
        D3D12_GPU_VIRTUAL_ADDRESS cbaddress;
    };

    struct material
    {
        // this can be used in constant buffers so order is deliberate
        vec3 fr = { 0.01f, 0.01f, 0.01f };
        float r = 0.25f;
        vec4 a = vec4::One;

        material& roughness(float _r) { r = _r; return *this; }
        material& diffusealbedo(vec4 const& _a) { a = _a; return *this; }
        material& fresnelr(vec3 const& _fr) { fr = _fr; return *this; }

        static material defaultmat;
    };

    struct instance_data
    {
        matrix matx;
        matrix invmatx;
        material mat;

        instance_data() = default;
        instance_data(matrix const& m, material const& _material) : matx(m.Transpose()), invmatx(m.Invert()), mat(_material) {}
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
        buffer objectconstant;
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