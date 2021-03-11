#pragma once

#include "Engine/EngineUtils.h"
#include "Engine/Interfaces/BodyInterface.h"
#include "Engine/SimpleMath.h"
#include "Engine/Shapes.h"

#include <memory>

using namespace DirectX;
using Microsoft::WRL::ComPtr;

namespace gfx
{
    struct instance_data
    {
        DirectX::SimpleMath::Vector3 position = {};
        DirectX::SimpleMath::Vector3 color = { 1.f, 0.f, 0.f};
    };

    template<typename body_type>
    class body_static : public body
    {
        body_type body;
        uint32_t num_instances;
        ComPtr<ID3D12Resource> m_vertexbuffer;
        ComPtr<ID3D12Resource> m_instance_buffer;
        uint8_t* m_instancebuffer_mapped = nullptr;
        std::vector<Geometry::Vertex> m_vertices;
        std::unique_ptr<instance_data[]> m_cpu_instance_data;

        static pipeline_objects pipelineobjects;
        static constexpr const wchar_t* c_ampshader_filename = L"InstancesAS.cso";
        static constexpr const wchar_t* c_meshshader_filename = L"InstancesMS.cso";
        static constexpr const wchar_t* c_pixelshader_filename = L"BasicLightingPS.cso";

        std::size_t get_vertexbuffersize() const { return m_vertices.size() * sizeof(Geometry::Vertex); }
        std::size_t get_instancebuffersize() const { return num_instances * sizeof(instance_data); }
    public:
        body_static(body_type const& _body, uint32_t _num_instances);
        void create_static_resources() override;
        std::vector<ComPtr<ID3D12Resource>> create_resources() override;
        D3D12_GPU_VIRTUAL_ADDRESS get_instancebuffer_gpuaddress() const;
        void update_instancebuffer();

        unsigned get_numvertices() const { return static_cast<unsigned>(m_vertices.size()); }
        pipeline_objects const& get_pipelineobjects() const override { return body_static<body_type>::get_static_pipelineobjects(); }
        D3D12_GPU_VIRTUAL_ADDRESS get_vertexbuffer_gpuaddress() const override { return m_vertexbuffer->GetGPUVirtualAddress(); }
        static pipeline_objects const& get_static_pipelineobjects() { return pipelineobjects; }
    };

    template<typename body_type>
    class body_dynamic : public body
    {
        body_type body;
        ComPtr<ID3D12Resource> m_vertexbuffer;
        uint8_t* m_vertexbuffer_databegin = nullptr;
        std::vector<Geometry::Vertex> m_vertices;

        static pipeline_objects pipelineobjects;
        static constexpr const wchar_t* c_ampshader_filename = L"DefaultAS.cso";
        static constexpr const wchar_t* c_meshshader_filename = L"DefaultMS.cso";
        static constexpr const wchar_t* c_pixelshader_filename = L"BasicLightingPS.cso";

        std::size_t get_vertexbuffersize() const { return m_vertices.size() * sizeof(Geometry::Vertex); }
    public:
        body_dynamic(body_type const& _body);
        void create_static_resources() override;
        std::vector<ComPtr<ID3D12Resource>> create_resources() override;
        void update(float dt) override;
        void update_vertexbuffer();

        unsigned get_numvertices() const { return static_cast<unsigned>(m_vertices.size()); }
        pipeline_objects const& get_pipelineobjects() const override { return body_dynamic<body_type>::get_static_pipelineobjects(); }
        D3D12_GPU_VIRTUAL_ADDRESS get_vertexbuffer_gpuaddress() const override;
        static pipeline_objects const& get_static_pipelineobjects() { return pipelineobjects; }
    };
}

#include "Body.hpp"