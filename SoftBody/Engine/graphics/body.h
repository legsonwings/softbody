#pragma once

#include "gfxcore.h"
#include "../../SharedConstants.h"
#include "engine/geometry/geocore.h"
#include "engine/engineutils.h"
#include "engine/interfaces/bodyinterface.h"
#include "gfxmemory.h"
#include "engine/SimpleMath.h"

#include <type_traits>
#include <unordered_map>

using namespace DirectX;
using Microsoft::WRL::ComPtr;

namespace gfx
{
    // todo : ensure these constraints can be satisfied by reference type instantiations
    // until then cannot constrain body templates
    template <typename t>
    concept sbody_c = requires(t v)
    {
        {v.gcenter()} -> std::convertible_to<vec3>;
        v.gvertices();
        {v.instancedata()} -> std::same_as<std::vector<instance_data>>;
    };

    template <typename t>
    concept dbody_c = requires(t v)
    {
        {v.gcenter()} -> std::convertible_to<vec3>;
        v.gvertices();
        v.update(float{});
    };

    template<topology prim_t = topology::triangle>
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

    template<typename body_t, topology prim_t>
    class body_static : public bodyinterface
    {
        using rawbody_t = std::decay_t<body_t>;
        using vertextype = typename topologyconstants<prim_t>::vertextype;

        body_t body;
        uint num_instances;
        ComPtr<ID3D12Resource> m_vertexbuffer;
        ComPtr<ID3D12Resource> m_instance_buffer;
        uint8_t* m_instancebuffer_mapped = nullptr;
        std::vector<vertextype> m_vertices;
        std::vector<instance_data> m_instancedata;

        using vertexfetch_r = decltype(m_vertices);
        using vertexfetch = std::function<vertexfetch_r(rawbody_t const&)>;
        using instancedatafetch_r = std::vector<instance_data>;
        using instancedatafetch = std::function<instancedatafetch_r(rawbody_t const&)>;

        vertexfetch get_vertices;
        instancedatafetch get_instancedata;

        uint get_vertexbuffersize() const { return m_vertices.size() * sizeof(vertextype); }
        uint get_instancebuffersize() const { return num_instances * sizeof(instance_data); }
        void update_instancebuffer();
        uint get_numinstances() const { return num_instances; }
        uint get_numvertices() const { return m_vertices.size(); }
        D3D12_GPU_VIRTUAL_ADDRESS get_instancebuffer_gpuaddress() const;
        D3D12_GPU_VIRTUAL_ADDRESS get_vertexbuffer_gpuaddress() const override { return m_vertexbuffer->GetGPUVirtualAddress(); }
    public:
        body_static(rawbody_t _body, bodyparams const & _params);
        body_static(body_t const& _body, vertexfetch_r(rawbody_t::* vfun)() const, instancedatafetch_r(rawbody_t::* ifun)() const, bodyparams const& _params);

        std::vector<ComPtr<ID3D12Resource>> create_resources() override;
        void render(float dt, renderparams const&) override;
        const geometry::aabb getaabb() const;

        constexpr body_t& get() { return body; }
        constexpr body_t const& get() const { return body; }
        constexpr body_t& operator*() { return body; }
        constexpr body_t const& operator*() const { return body; }
        constexpr rawbody_t* operator->() { return &body; }
        constexpr rawbody_t const* operator->() const { return &body; }
    };

    template<typename body_t, topology prim_t>
    class body_dynamic : public bodyinterface
    {
        using rawbody_t = std::decay_t<body_t>;
        using vertextype = typename topologyconstants<prim_t>::vertextype;

        body_t body;
        constant_buffer cbuffer;
        ComPtr<ID3D12Resource> m_vertexbuffer;
        uint8_t* m_vertexbuffer_databegin = nullptr;
        std::vector<vertextype> m_vertices;
        
        using vertexfetch_r = decltype(m_vertices);
        using vertexfetch = std::function<vertexfetch_r (rawbody_t const&)>;

        vertexfetch get_vertices;

        void update_constbuffer();
        void update_vertexbuffer();
        unsigned get_numvertices() const { return static_cast<unsigned>(m_vertices.size()); }
        uint get_vertexbuffersize() const { return m_vertices.size() * sizeof(vertextype); }
        D3D12_GPU_VIRTUAL_ADDRESS get_vertexbuffer_gpuaddress() const override;
    public:
        body_dynamic(rawbody_t _body, bodyparams const& _params);
        body_dynamic(body_t const& _body, vertexfetch_r (rawbody_t::*fun)() const, bodyparams const& _params);

        std::vector<ComPtr<ID3D12Resource>> create_resources() override;

        void update(float dt) override;
        void render(float dt, renderparams const&) override;

        constexpr body_t &get() { return body; } 
        constexpr body_t const &get() const { return body; }
        constexpr body_t &operator*() { return body; }
        constexpr body_t const &operator*() const { return body; }
        constexpr rawbody_t *operator->() { return &body; }
        constexpr rawbody_t const *operator->() const { return &body; }
    };
}

#include "body.hpp"