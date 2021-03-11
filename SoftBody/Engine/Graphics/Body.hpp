#include "Body.h"
#include "SharedConstants.h"

#include <string>
namespace gfx
{
    struct shader
    {
        byte* data;
        uint32_t size;
    };

    using default_and_upload_buffers = std::pair<ComPtr<ID3D12Resource>, ComPtr<ID3D12Resource>>;

    pipeline_objects create_pipelineobjects(std::wstring const& as, std::wstring const& ms, std::wstring const& ps);
    ComPtr<ID3D12Resource> create_uploadbuffer(uint8_t** mapped_buffer, std::size_t const buffersize);
    default_and_upload_buffers create_vertexbuffer_default(void* vertexdata_start, std::size_t const vb_size);
    ComPtr<ID3D12Resource> create_vertexbuffer_upload(uint8_t** mapped_buffer, void* vertexdata_start, std::size_t const perframe_buffersize);
    ComPtr<ID3D12Resource> create_instancebuffer(uint8_t** mapped_buffer, void* data_start, std::size_t const perframe_buffersize);
    D3D12_GPU_VIRTUAL_ADDRESS get_perframe_gpuaddress(D3D12_GPU_VIRTUAL_ADDRESS start, UINT64 perframe_buffersize);
    void update_perframebuffer(uint8_t* mapped_buffer, void* data_start, std::size_t const perframe_buffersize);

    template<typename body_type>
    pipeline_objects body_static<body_type>::pipelineobjects;

    template<typename body_type>
    body_static<body_type>::body_static(body_type const& _body, uint32_t _num_instances) : body(_body), num_instances(_num_instances)
    {
        create_static_resources();
        m_cpu_instance_data.reset(new instance_data[num_instances]);
    }

    template<typename body_type>
    inline void body_static<body_type>::create_static_resources()
    {
        if(!pipelineobjects.root_signature)
            pipelineobjects = create_pipelineobjects(c_ampshader_filename, c_meshshader_filename, c_pixelshader_filename);
    }

    template<typename body_type>
    std::vector<ComPtr<ID3D12Resource>> body_static<body_type>::create_resources()
    {
        m_vertices = body.GetTriangles();
        assert(m_vertices.size() < AS_GROUP_SIZE * MAX_MSGROUPS_PER_ASGROUP * MAX_TRIANGLES_PER_GROUP * 3);

        // todo : refactor this
        m_cpu_instance_data[0].position = body.center;
        m_cpu_instance_data[0].color = { 1.f, 0.f, 0.f};

        auto const vb_resources = create_vertexbuffer_default(m_vertices.data(), m_vertices.size() * sizeof(Geometry::Vertex));
        m_vertexbuffer = vb_resources.first;

        m_instance_buffer = create_instancebuffer(&m_instancebuffer_mapped, m_cpu_instance_data.get(), get_instancebuffersize());

        // return the upload buffer so that engine can keep it alive until vertex data has been uploaded to gpu
        return { vb_resources.second };
    }

    template<typename body_type>
    inline D3D12_GPU_VIRTUAL_ADDRESS body_static<body_type>::get_instancebuffer_gpuaddress() const
    {
        return get_perframe_gpuaddress(m_instance_buffer->GetGPUVirtualAddress(), get_instancebuffersize());
    }

    template<typename body_type>
    inline void body_static<body_type>::update_instancebuffer()
    {
        update_perframebuffer(m_instancebuffer_mapped, m_cpu_instance_data.get(), get_instancebuffersize());
    }

    template<typename body_type>
    pipeline_objects body_dynamic<body_type>::pipelineobjects;

    template<typename body_type>
    body_dynamic<body_type>::body_dynamic(body_type const& _body) : body(_body)
    {
        create_static_resources();
    }

    template<typename body_type>
    inline void body_dynamic<body_type>::create_static_resources()
    {
        if(!pipelineobjects.root_signature)
            pipelineobjects = create_pipelineobjects(c_ampshader_filename, c_meshshader_filename, c_pixelshader_filename);
    }

    template<typename body_type>
    std::vector<ComPtr<ID3D12Resource>> body_dynamic<body_type>::create_resources()
    {
        pipelineobjects = create_pipelineobjects(c_ampshader_filename, c_meshshader_filename, c_pixelshader_filename);

        m_vertices = body.get_vertices();
        assert(m_vertices.size() < AS_GROUP_SIZE * MAX_MSGROUPS_PER_ASGROUP * MAX_TRIANGLES_PER_GROUP * 3);

        m_vertexbuffer = create_vertexbuffer_upload(&m_vertexbuffer_databegin, m_vertices.data(), get_vertexbuffersize());
        return std::vector<ComPtr<ID3D12Resource>>();
    }

    template<typename body_type>
    void body_dynamic<body_type>::update(float dt)
    {
        body.update(dt);
        m_vertices = body.get_vertices();
        assert(m_vertices.size() < AS_GROUP_SIZE * MAX_MSGROUPS_PER_ASGROUP * MAX_TRIANGLES_PER_GROUP * 3);
    }

    template<typename body_type>
    void body_dynamic<body_type>::update_vertexbuffer()
    {
        update_perframebuffer(m_vertexbuffer_databegin, m_vertices.data(), get_vertexbuffersize());
    }

    template<typename body_type>
    inline D3D12_GPU_VIRTUAL_ADDRESS body_dynamic<body_type>::get_vertexbuffer_gpuaddress() const
    {
        return get_perframe_gpuaddress(m_vertexbuffer->GetGPUVirtualAddress(), get_vertexbuffersize());
    }
}