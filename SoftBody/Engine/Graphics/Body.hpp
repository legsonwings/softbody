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
    void update_allframebuffers(uint8_t* mapped_buffer, void* data_start, std::size_t const perframe_buffersize, std::size_t framecount);

    template<typename body_type, gfx::topology primitive_type>
    pipeline_objects body_static<body_type, primitive_type>::pipelineobjects;


    template<typename body_type, gfx::topology primitive_type>
    inline body_static<body_type, primitive_type>::body_static(body_type _body, vertex_fetch const& _get_vertices, instancedata_fetch const& _get_instancedata)
        : body(_body), get_vertices(_get_vertices), get_instancedata(_get_instancedata)
    {
        create_static_resources();
    }

    template<typename body_type, gfx::topology primitive_type>
    inline void body_static<body_type, primitive_type>::create_static_resources()
    {
        if(!pipelineobjects.root_signature)
            pipelineobjects = create_pipelineobjects(c_ampshader_filename<primitive_type>, c_meshshader_filename<primitive_type>, c_pixelshader_filename<primitive_type>);
    }

    template<typename body_type, gfx::topology primitive_type>
    std::vector<ComPtr<ID3D12Resource>> body_static<body_type, primitive_type>::create_resources()
    {
        m_vertices = get_vertices(body);
        auto instances = get_instancedata(body);
        num_instances = instances.size();
        m_cpu_instance_data.reset(new instance_data[num_instances]);

        std::copy(instances.begin(), instances.end(), m_cpu_instance_data.get());

        assert(m_vertices.size() < AS_GROUP_SIZE * MAX_MSGROUPS_PER_ASGROUP * MAX_TRIANGLES_PER_GROUP * 3);

        auto const vb_resources = create_vertexbuffer_default(m_vertices.data(), m_vertices.size() * sizeof(decltype(m_vertices)::value_type));
        m_vertexbuffer = vb_resources.first;

        m_instance_buffer = create_instancebuffer(&m_instancebuffer_mapped, m_cpu_instance_data.get(), get_instancebuffersize());

        // return the upload buffer so that engine can keep it alive until vertex data has been uploaded to gpu
        return { vb_resources.second };
    }

    template<typename body_type, gfx::topology primitive_type>
    inline D3D12_GPU_VIRTUAL_ADDRESS body_static<body_type, primitive_type>::get_instancebuffer_gpuaddress() const
    {
        return get_perframe_gpuaddress(m_instance_buffer->GetGPUVirtualAddress(), get_instancebuffersize());
    }

    template<typename body_type, gfx::topology primitive_type>
    inline void body_static<body_type, primitive_type>::update_instancebuffer()
    {
        auto instances = get_instancedata(body);

        std::copy(instances.begin(), instances.end(), m_cpu_instance_data.get());
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