#pragma once

#include "gfxcore.h"
#include "Engine/geometry/geocore.h"
#include "Engine/engineutils.h"
#include "Engine/interfaces/bodyinterface.h"
#include "Engine/SimpleMath.h"
#include "Engine/geometry/Shapes.h"

#include <type_traits>
#include <memory>

using namespace DirectX;
using Microsoft::WRL::ComPtr;

namespace gfx
{
    template<typename geometry_t, gfx::topology primitive_t>
    class body_static : public body
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

        static pipeline_objects pipelineobjects;

        template<gfx::topology primitive_t> static constexpr uint32_t vb_slot = 2;
        template<> static constexpr uint32_t vb_slot<gfx::topology::line> = 3;
        template<gfx::topology primitive_t> static constexpr uint32_t numverts_perprim = 3;
        template<> static constexpr uint32_t numverts_perprim<gfx::topology::line> = 2;
        template<gfx::topology primitive_t> static constexpr vec3 color = { 1.f, 0.3f, 0.1f };
        template<> static constexpr vec3 color<gfx::topology::line> = { 1.f, 1.f, 0.f };

        template<gfx::topology primitive_t> static constexpr  wchar_t* c_ampshader_filename = L"InstancesAS.cso";
        template<gfx::topology primitive_t> static constexpr const wchar_t* c_meshshader_filename = L"InstancesMS.cso";
        template<gfx::topology primitive_t> static constexpr const wchar_t* c_pixelshader_filename = L"BasicLightingPS.cso";

        template<> static constexpr const wchar_t* c_ampshader_filename<gfx::topology::line> = L"";
        template<> static constexpr const wchar_t* c_meshshader_filename<gfx::topology::line> = L"linesinstancesMS.cso";
        template<> static constexpr const wchar_t* c_pixelshader_filename<gfx::topology::line> = L"BasicPS.cso";

        uint get_vertexbuffersize() const { return m_vertices.size() * sizeof(decltype(m_vertices)::value_type); }
        uint get_instancebuffersize() const { return num_instances * sizeof(instance_data); }
    public:
        template<typename = std::enable_if_t<!std::is_lvalue_reference_v<geometry_t>>>
        body_static(geometry_t _body);
        body_static(geometry_t const& _body, vertexfetch_r(std::decay_t<geometry_t>::* vfun)() const, instancedatafetch_r(std::decay_t<geometry_t>::* ifun)() const);

        std::vector<ComPtr<ID3D12Resource>> create_resources() override;
        void render(float dt, renderparams const&) override;
        void update_instancebuffer();

        uint get_numinstances() const { return num_instances; }
        uint get_numvertices() const { return m_vertices.size(); }
        pipeline_objects const& get_pipelineobjects() const override { return body_static<geometry_t, primitive_t>::get_static_pipelineobjects(); }
        D3D12_GPU_VIRTUAL_ADDRESS get_instancebuffer_gpuaddress() const;
        D3D12_GPU_VIRTUAL_ADDRESS get_vertexbuffer_gpuaddress() const override { return m_vertexbuffer->GetGPUVirtualAddress(); }
        static pipeline_objects const& get_static_pipelineobjects();
    };

    template<typename geometry_t, gfx::topology primitive_t>
    class body_dynamic : public body
    {
        static_assert(!std::is_rvalue_reference_v<geometry_t>, "storing rvalue reference types is prohibited.");

        geometry_t body;
        ComPtr<ID3D12Resource> m_vertexbuffer;
        uint8_t* m_vertexbuffer_databegin = nullptr;
        std::vector<std::conditional_t<primitive_t == gfx::topology::triangle, geometry::vertex, vec3>> m_vertices;
        
        using vertexfetch_r = decltype(m_vertices);
        using vertexfetch = std::function<vertexfetch_r (std::decay_t<geometry_t> const&)>;

        vertexfetch get_vertices;

        static pipeline_objects pipelineobjects;

        template<gfx::topology primitive_t> static constexpr uint32_t vb_slot = 2;
        template<> static constexpr uint32_t vb_slot<gfx::topology::line> = 3;
        template<gfx::topology primitive_t> static constexpr uint32_t numverts_perprim = 3;
        template<> static constexpr uint32_t numverts_perprim<gfx::topology::line> = 2;
        template<gfx::topology primitive_t> static constexpr vec3 color = { 1.f, 0.3f, 0.1f };
        template<> static constexpr vec3 color<gfx::topology::line> = { 1.f, 1.f, 0.f };

        template<gfx::topology primitive_t> static constexpr const wchar_t* c_ampshader_filename = L"DefaultAS.cso";
        template<gfx::topology primitive_t> static constexpr const wchar_t* c_meshshader_filename = L"DefaultMS.cso";
        template<gfx::topology primitive_t> static constexpr const wchar_t* c_pixelshader_filename = L"BasicLightingPS.cso";

        template<> static constexpr const wchar_t* c_ampshader_filename<gfx::topology::line> = L"";
        template<> static constexpr const wchar_t* c_meshshader_filename<gfx::topology::line> = L"linesMS.cso";
        template<> static constexpr const wchar_t* c_pixelshader_filename<gfx::topology::line> = L"BasicPS.cso";

        uint get_vertexbuffersize() const { return m_vertices.size() * sizeof(decltype(m_vertices)::value_type); }
    public:
        
        template<typename = std::enable_if_t<!std::is_lvalue_reference_v<geometry_t>>>
        body_dynamic(geometry_t _body);
        body_dynamic(geometry_t const& _body, vertexfetch_r (std::decay_t<geometry_t>::*fun)() const);

        std::vector<ComPtr<ID3D12Resource>> create_resources() override;

        void update(float dt) override;
        void render(float dt, renderparams const&) override;
        void update_vertexbuffer();

        constexpr geometry_t &get() { return body; }
        constexpr geometry_t const &get() const { return body; }
        constexpr geometry_t &operator*() { return body; }
        constexpr geometry_t const &operator*() const { return body; }
        constexpr geometry_t &operator->() { return body; }
        constexpr geometry_t const &operator->() const { return body; }

        unsigned get_numvertices() const { return static_cast<unsigned>(m_vertices.size()); }
        pipeline_objects const& get_pipelineobjects() const override { return body_dynamic<geometry_t>::get_static_pipelineobjects(); }
        D3D12_GPU_VIRTUAL_ADDRESS get_vertexbuffer_gpuaddress() const override;
        static pipeline_objects const& get_static_pipelineobjects();
    };
}

#include "Body.hpp"