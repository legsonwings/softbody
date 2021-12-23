#pragma once

#include "engine/core.h"
#include "engine/sharedconstants.h"
#include "engine/engineutils.h"
#include "engine/simplemath.h"

#include <wrl.h>
#include <string>
#include "d3dx12.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

struct ID3D12PipelineState;
struct ID3D12RootSignature;

namespace gfx
{
    using resourcelist = std::vector<ComPtr<ID3D12Resource>>;

    enum class topology
    {
        triangle,
        line,
    };

    enum psoflags
    {
        none = 0x0,
        wireframe = 0x1,
        transparent = 0x2,
        twosided = 0x4,
        count
    };

    struct color
    {
        static inline vector4 red = vector4{ 1.f, 0.f, 0.f, 1.f };
        static inline vector4 black = vector4{ 0.f, 0.f, 0.f, 1.f };
        static inline vector4 white = vector4{ 1.f, 1.f, 1.f, 1.f };
    };

    struct shader
    {
        byte* data;
        uint32_t size;
    };

    struct renderparams
    {
        bool wireframe = false;
    };

    struct buffer
    {
        uint32_t slot;
        D3D12_GPU_DESCRIPTOR_HANDLE deschandle = { 0 };
    };

    struct rootbuffer
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
        rootbuffer constant;
        rootbuffer objectconstant;
        rootbuffer vertex;
        rootbuffer instance;
        buffer texture;
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
        vector3 fr = { 0.01f, 0.01f, 0.01f };
        float r = 0.25f;
        vector4 a = vector4::One;

        material& roughness(float _r) { r = _r; return *this; }
        material& diffuse(vector4 const& _a) { a = _a; return *this; }
        material& fresnelr(vector3 const& _fr) { fr = _fr; return *this; }
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

    // todo : replace with std::alignas
    _declspec(align(256u)) struct objectconstants : public instance_data
    {
        objectconstants() = default;
        objectconstants(matrix const& m, viewinfo const& v, material const& _material) : instance_data(m, v, _material) {}
    };

    struct light
    {
        vector3 color;
        float range;
        vector3 position;
        uint8_t padding1[4];
        vector3 direction;
        uint8_t padding2[4];
    };

    _declspec(align(256u)) struct sceneconstants
    {
        vector3 campos;
        uint8_t padding[4];
        vector4 ambient;
        light lights[MAX_NUM_LIGHTS];
        uint32_t numdirlights = 0;
        uint32_t numpointlights;
    };
}