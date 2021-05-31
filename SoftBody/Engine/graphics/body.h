#pragma once

#include "gfxcore.h"
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
    template<typename body_type, gfx::topology primitive_type>
    class body_static : public body
    {
        body_type body;
        std::size_t num_instances;
        ComPtr<ID3D12Resource> m_vertexbuffer;
        ComPtr<ID3D12Resource> m_instance_buffer;
        uint8_t* m_instancebuffer_mapped = nullptr;
        std::vector<std::conditional_t<primitive_type == gfx::topology::triangle, geometry::vertex, vec3>> m_vertices;
        std::unique_ptr<instance_data[]> m_cpu_instance_data;

        using vertex_fetch = std::function<decltype(m_vertices)(body_type const&)>;
        using instancedata_fetch = std::function<std::vector<instance_data>(body_type const&)>;
        vertex_fetch get_vertices;
        instancedata_fetch get_instancedata;

        static pipeline_objects pipelineobjects;

        template<gfx::topology primitive_type>
        static constexpr const wchar_t* c_ampshader_filename = L"InstancesAS.cso";
        template<gfx::topology primitive_type>
        static constexpr const wchar_t* c_meshshader_filename = L"InstancesMS.cso";
        template<gfx::topology primitive_type>
        static constexpr const wchar_t* c_pixelshader_filename = L"BasicLightingPS.cso";

        template<>
        static constexpr const wchar_t* c_ampshader_filename<gfx::topology::line> = L"";
        template<>
        static constexpr const wchar_t* c_meshshader_filename<gfx::topology::line> = L"LinesMS.cso";
        template<>
        static constexpr const wchar_t* c_pixelshader_filename<gfx::topology::line> = L"BasicPS.cso";

        std::size_t get_vertexbuffersize() const { return m_vertices.size() * sizeof(decltype(m_vertices)::value_type); }
        std::size_t get_instancebuffersize() const { return num_instances * sizeof(instance_data); }
    public:
        body_static(body_type _body, vertex_fetch const&, instancedata_fetch const&);
        void create_static_resources() override;
        std::vector<ComPtr<ID3D12Resource>> create_resources() override;
        D3D12_GPU_VIRTUAL_ADDRESS get_instancebuffer_gpuaddress() const;
        void update_instancebuffer();

        unsigned get_numinstances() const { return static_cast<unsigned>(num_instances); }
        unsigned get_numvertices() const { return static_cast<unsigned>(m_vertices.size()); }
        pipeline_objects const& get_pipelineobjects() const override { return body_static<body_type, primitive_type>::get_static_pipelineobjects(); }
        D3D12_GPU_VIRTUAL_ADDRESS get_vertexbuffer_gpuaddress() const override { return m_vertexbuffer->GetGPUVirtualAddress(); }
        static pipeline_objects const& get_static_pipelineobjects() { return pipelineobjects; }
    };

    template<typename body_type>
    class body_dynamic : public body
    {
        body_type body;
        ComPtr<ID3D12Resource> m_vertexbuffer;
        uint8_t* m_vertexbuffer_databegin = nullptr;
        std::vector<geometry::vertex> m_vertices;

        static pipeline_objects pipelineobjects;
        static constexpr const wchar_t* c_ampshader_filename = L"DefaultAS.cso";
        static constexpr const wchar_t* c_meshshader_filename = L"DefaultMS.cso";
        static constexpr const wchar_t* c_pixelshader_filename = L"BasicLightingPS.cso";

        std::size_t get_vertexbuffersize() const { return m_vertices.size() * sizeof(geometry::vertex); }
    public:
        body_dynamic(body_type const& _body);
        void create_static_resources() override;
        std::vector<ComPtr<ID3D12Resource>> create_resources() override;
        void update(float dt) override;
        void update_vertexbuffer();

        body_type const& get_body() const { return body; }
        body_type & get_body() { return body; }
        unsigned get_numvertices() const { return static_cast<unsigned>(m_vertices.size()); }
        pipeline_objects const& get_pipelineobjects() const override { return body_dynamic<body_type>::get_static_pipelineobjects(); }
        D3D12_GPU_VIRTUAL_ADDRESS get_vertexbuffer_gpuaddress() const override;
        static pipeline_objects const& get_static_pipelineobjects() { return pipelineobjects; }
    };
}

#include "Body.hpp"