#pragma once

#include "gfxcore.h"
#include "engine/geometry/geocore.h"
#include "engine/engineutils.h"
#include "engine/interfaces/bodyinterface.h"
#include "engine/SimpleMath.h"
#include "engine/geometry/shapes.h"

#include <type_traits>
#include <memory>
#include <unordered_map>

using namespace DirectX;
using Microsoft::WRL::ComPtr;

namespace gfx
{
    template<typename geometry_t, gfx::topology primitive_t>
    class body_static : public bodyinterface
    {
        geometry_t body;
        uint num_instances;
        ComPtr<ID3D12Resource> m_vertexbuffer;
        ComPtr<ID3D12Resource> m_instance_buffer;
        uint8_t* m_instancebuffer_mapped = nullptr;
        std::vector<std::conditional_t<primitive_t == gfx::topology::triangle, geometry::vertex, vec3>> m_vertices;
        std::unique_ptr<instance_data[]> m_cpu_instance_data;

        using vertexfetch_r = decltype(m_vertices);
        using vertexfetch = std::function<vertexfetch_r (std::decay_t<geometry_t> const&)>;
        using instancedatafetch_r = std::vector<instance_data>;
        using instancedatafetch = std::function<instancedatafetch_r(std::decay_t<geometry_t> const&)>;

        vertexfetch get_vertices;
        instancedatafetch get_instancedata;

        template<gfx::topology primitive_t> static constexpr uint32_t vb_slot = 2;
        template<> static constexpr uint32_t vb_slot<gfx::topology::line> = 3;
        template<gfx::topology primitive_t> static constexpr uint32_t numverts_perprim = 3;
        template<> static constexpr uint32_t numverts_perprim<gfx::topology::line> = 2;
        template<gfx::topology primitive_t> static constexpr vec3 color = { 1.f, 0.3f, 0.1f };
        template<> static constexpr vec3 color<gfx::topology::line> = { 1.f, 1.f, 0.f };

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

    template<typename geometry_t, gfx::topology primitive_t>
    class body_dynamic : public bodyinterface
    {
        static_assert(!std::is_rvalue_reference_v<geometry_t>, "storing rvalue reference types is prohibited.");

        geometry_t body;
        ComPtr<ID3D12Resource> m_vertexbuffer;
        uint8_t* m_vertexbuffer_databegin = nullptr;
        std::vector<std::conditional_t<primitive_t == gfx::topology::triangle, geometry::vertex, vec3>> m_vertices;
        
        using vertexfetch_r = decltype(m_vertices);
        using vertexfetch = std::function<vertexfetch_r (std::decay_t<geometry_t> const&)>;

        vertexfetch get_vertices;

        template<gfx::topology primitive_t> static constexpr uint32_t vb_slot = 2;
        template<> static constexpr uint32_t vb_slot<gfx::topology::line> = 3;
        template<gfx::topology primitive_t> static constexpr uint32_t numverts_perprim = 3;
        template<> static constexpr uint32_t numverts_perprim<gfx::topology::line> = 2;
        template<gfx::topology primitive_t> static constexpr vec3 color = { 1.f, 0.3f, 0.1f };
        template<> static constexpr vec3 color<gfx::topology::line> = { 1.f, 1.f, 0.f };

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