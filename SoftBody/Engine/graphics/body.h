#pragma once

#include "gfxcore.h"
#include "../../SharedConstants.h"
#include "engine/geometry/geocore.h"
#include "engine/engineutils.h"
#include "engine/interfaces/bodyinterface.h"
#include "gfxmemory.h"
#include "engine/SimpleMath.h"
#include "engine/geometry/shapes.h"

#include <type_traits>
#include <memory>
#include <unordered_map>

using namespace DirectX;
using Microsoft::WRL::ComPtr;

namespace gfx
{
    _declspec(align(256u)) struct objectconstants
    {
        matrix matx;
        matrix invmatx;
        material mat;
        objectconstants() = default;
        objectconstants(matrix const& m, material const& _material) : matx(m.Transpose()), invmatx(m.Invert()), mat(_material) {}
    };

    template<topology primitive_t = topology::triangle>
    struct topologyconstants
    {
        using vertextype = geometry::vertex;
        static constexpr uint32_t numverts_perprim = 3;
        static constexpr uint32_t maxprims_permsgroup = MAX_TRIANGLES_PER_GROUP;
    };

    template<>
    struct topologyconstants<topology::line>
    {
        using vertextype = vec3;
        static constexpr uint32_t numverts_perprim = 2;
        static constexpr uint32_t maxprims_permsgroup = MAX_LINES_PER_GROUP;
    };

    template<typename geometry_t, topology primitive_t>
    class body_static : public bodyinterface
    {
        geometry_t body;
        uint num_instances;
        ComPtr<ID3D12Resource> m_vertexbuffer;
        ComPtr<ID3D12Resource> m_instance_buffer;
        uint8_t* m_instancebuffer_mapped = nullptr;
        std::vector<typename topologyconstants<primitive_t>::vertextype> m_vertices;
        std::unique_ptr<instance_data[]> m_cpu_instance_data;

        using vertexfetch_r = decltype(m_vertices);
        using vertexfetch = std::function<vertexfetch_r (std::decay_t<geometry_t> const&)>;
        using instancedatafetch_r = std::vector<instance_data>;
        using instancedatafetch = std::function<instancedatafetch_r(std::decay_t<geometry_t> const&)>;

        vertexfetch get_vertices;
        instancedatafetch get_instancedata;

        uint get_vertexbuffersize() const { return m_vertices.size() * sizeof(decltype(m_vertices)::value_type); }
        uint get_instancebuffersize() const { return num_instances * sizeof(instance_data); }
        void update_instancebuffer();
        uint get_numinstances() const { return num_instances; }
        uint get_numvertices() const { return m_vertices.size(); }
        D3D12_GPU_VIRTUAL_ADDRESS get_instancebuffer_gpuaddress() const;
        D3D12_GPU_VIRTUAL_ADDRESS get_vertexbuffer_gpuaddress() const override { return m_vertexbuffer->GetGPUVirtualAddress(); }
    public:
        template<typename = std::enable_if_t<!std::is_lvalue_reference_v<geometry_t>>>
        body_static(geometry_t _body, bodyparams const & _params);
        body_static(geometry_t const& _body, vertexfetch_r(std::decay_t<geometry_t>::* vfun)() const, instancedatafetch_r(std::decay_t<geometry_t>::* ifun)() const, bodyparams const& _params);

        std::vector<ComPtr<ID3D12Resource>> create_resources() override;
        void render(float dt, renderparams const&) override;
    };

    template<typename geometry_t, topology primitive_t>
    class body_dynamic : public bodyinterface
    {
        static_assert(!std::is_rvalue_reference_v<geometry_t>, "storing rvalue reference types is prohibited.");

        geometry_t body;
        constant_buffer cbuffer;
        ComPtr<ID3D12Resource> m_vertexbuffer;
        // todo : move these to upload buffer helper type
        uint8_t* m_vertexbuffer_databegin = nullptr;
        std::vector<typename topologyconstants<primitive_t>::vertextype> m_vertices;
        
        using vertexfetch_r = decltype(m_vertices);
        using vertexfetch = std::function<vertexfetch_r (std::decay_t<geometry_t> const&)>;

        vertexfetch get_vertices;

        void update_constbuffer();
        void update_vertexbuffer();
        unsigned get_numvertices() const { return static_cast<unsigned>(m_vertices.size()); }
        uint get_vertexbuffersize() const { return m_vertices.size() * sizeof(decltype(m_vertices)::value_type); }
        D3D12_GPU_VIRTUAL_ADDRESS get_vertexbuffer_gpuaddress() const override;
    public:
        
        template<typename = std::enable_if_t<!std::is_lvalue_reference_v<geometry_t>>>
        body_dynamic(geometry_t _body, bodyparams const& _params);
        body_dynamic(geometry_t const& _body, vertexfetch_r (std::decay_t<geometry_t>::*fun)() const, bodyparams const& _params);

        std::vector<ComPtr<ID3D12Resource>> create_resources() override;

        void update(float dt) override;
        void render(float dt, renderparams const&) override;

        constexpr geometry_t &get() { return body; }
        constexpr geometry_t const &get() const { return body; }
        constexpr geometry_t &operator*() { return body; }
        constexpr geometry_t const &operator*() const { return body; }
        constexpr std::decay_t<geometry_t> *operator->() { return &body; }
        constexpr std::decay_t<geometry_t> const *operator->() const { return &body; }
    };
}

#include "body.hpp"