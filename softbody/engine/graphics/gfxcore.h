#pragma once

#include "engine/geometry/geodefines.h"
#include "engine/sharedconstants.h"
#include "engine/engineutils.h"
#include "engine/SimpleMath.h"

#include <wrl.h>
#include <string>
#include "d3dx12.h"

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
        twosided = 0x4,
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
    
    struct pipeline_objects
    {
        ComPtr<ID3D12PipelineState> pso;
        ComPtr<ID3D12PipelineState> pso_wireframe;
        ComPtr<ID3D12PipelineState> pso_twosided;
        ComPtr<ID3D12RootSignature> root_signature;
    };

    struct resource_bindings
    {
        buffer constant;
        buffer objectconstant;
        buffer vertex;
        buffer instance;
        rootconstants rootconstants;
        pipeline_objects pipelineobjs;
    };

    struct viewinfo
    {
        matrix view;
        matrix proj;
    };

    // these are passed to gpu, so be careful about alignment and padding
    struct material
    {
        // the member order is deliberate
        vec3 fr = { 0.01f, 0.01f, 0.01f };
        float r = 0.25f;
        vec4 a = vec4::One;

        material& roughness(float _r) { r = _r; return *this; }
        material& diffusealbedo(vec4 const& _a) { a = _a; return *this; }
        material& fresnelr(vec3 const& _fr) { fr = _fr; return *this; }
    };

    // this is used in constant buffer so alignment is important
    struct instance_data
    {
        matrix matx;
        matrix normalmatx;
        matrix mvpmatx;
        material mat;
        instance_data() = default;
        instance_data(matrix const& m, viewinfo const& v, material const& _material) 
            : matx(m.Transpose()), normalmatx(m.Invert()), mvpmatx((m* v.view* v.proj).Transpose()), mat(_material) {}
    };

    _declspec(align(256u)) struct objectconstants : public instance_data
    {
        objectconstants() = default;
        objectconstants(matrix const& m, viewinfo const& v, material const& _material) : instance_data(m, v, _material) {}
    };

    struct light
    {
        vec3 color;
        float range;
        vec3 position;
        uint8_t padding1[4];
        vec3 direction;
        uint8_t padding2[4];
    };

    _declspec(align(256u)) struct sceneconstants
    {
        vec3 campos;
        uint8_t padding[4];
        vec4 ambient;
        light lights[NUM_LIGHTS];
    };
}