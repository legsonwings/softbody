#include "body.h"
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

    template<typename geometry_t, gfx::topology primitive_t>
    pipeline_objects body_static<geometry_t, primitive_t>::pipelineobjects;


    template<typename geometry_t, gfx::topology primitive_t>
    template<typename>
    inline body_static<geometry_t, primitive_t>::body_static(geometry_t _body) : body(_body) 
    {
        get_vertices = [](geometry_t const& geom) { return geom.get_vertices(); };
    }

    template<typename geometry_t, gfx::topology primitive_t>
    inline body_static<geometry_t, primitive_t>::body_static(geometry_t const& _body, vertexfetch_r (std::decay_t<geometry_t>::* vfun)() const, instancedatafetch_r (std::decay_t<geometry_t>::* ifun)() const) : body(_body)
    {
        get_vertices = [vfun](geometry_t const& geom) { return (geom.*vfun)(); };
        get_instancedata = [ifun](geometry_t const& geom) { return (geom.*ifun)(); };
    }

    template<typename geometry_t, gfx::topology primitive_t>
    std::vector<ComPtr<ID3D12Resource>> body_static<geometry_t, primitive_t>::create_resources()
    {
        m_vertices = get_vertices(body);
        auto instances = get_instancedata(body);
        num_instances = instances.size();
        m_cpu_instance_data.reset(new instance_data[num_instances]);

        std::copy(instances.begin(), instances.end(), m_cpu_instance_data.get());

        assert(m_vertices.size() < AS_GROUP_SIZE * MAX_MSGROUPS_PER_ASGROUP * MAX_TRIANGLES_PER_GROUP * 3);

        auto const vb_resources = create_vertexbuffer_default(m_vertices.data(), get_vertexbuffersize());
        m_vertexbuffer = vb_resources.first;

        m_instance_buffer = create_instancebuffer(&m_instancebuffer_mapped, m_cpu_instance_data.get(), get_instancebuffersize());

        // return the upload buffer so that engine can keep it alive until vertex data has been uploaded to gpu
        return { vb_resources.second };
    }

    template<typename geometry_t, gfx::topology primitive_t>
    inline pipeline_objects const& body_static<geometry_t, primitive_t>::get_static_pipelineobjects()
    {
        if (!pipelineobjects.root_signature)
            pipelineobjects = create_pipelineobjects(c_ampshader_filename<primitive_t>, c_meshshader_filename<primitive_t>, c_pixelshader_filename<primitive_t>);

        return pipelineobjects;
    }

    template<typename geometry_t, gfx::topology primitive_t>
    inline D3D12_GPU_VIRTUAL_ADDRESS body_static<geometry_t, primitive_t>::get_instancebuffer_gpuaddress() const
    {
        return get_perframe_gpuaddress(m_instance_buffer->GetGPUVirtualAddress(), get_instancebuffersize());
    }

    template<typename geometry_t, gfx::topology primitive_t>
    inline void body_static<geometry_t, primitive_t>::update_instancebuffer()
    {
        auto instances = get_instancedata(body);

        std::copy(instances.begin(), instances.end(), m_cpu_instance_data.get());
        update_perframebuffer(m_instancebuffer_mapped, m_cpu_instance_data.get(), get_instancebuffersize());
    }

    template<typename geometry_t, gfx::topology primitive_t>
    pipeline_objects body_dynamic<geometry_t, primitive_t>::pipelineobjects;

    template<typename geometry_t, gfx::topology primitive_t>
    template<typename>
    inline body_dynamic<geometry_t, primitive_t>::body_dynamic(geometry_t _body)  : body(_body)
    {
        get_vertices = [](geometry_t const& geom) { return geom.get_vertices(); };
    }

    template<typename geometry_t, gfx::topology primitive_t>
    inline body_dynamic<geometry_t, primitive_t>::body_dynamic(geometry_t const& _body, vertexfetch_r (std::decay_t<geometry_t>::*fun)() const) : body(_body)
    {
        get_vertices = [fun](geometry_t const& geom) { return (geom.*fun)(); };

        // todo : add support for data member pointers
        /*if constexpr (std::is_member_function_pointer_v<decltype(f)>)
        {
            
        }
        else
        {
            get_vertices = [f](geometry_t const& geom) { return geom.*f; };
        }*/
    }

    template<typename geometry_t, gfx::topology primitive_t>
    inline pipeline_objects const& body_dynamic<geometry_t, primitive_t>::get_static_pipelineobjects()
    {
        if (!pipelineobjects.root_signature)
            pipelineobjects = create_pipelineobjects(c_ampshader_filename<primitive_t>, c_meshshader_filename<primitive_t>, c_pixelshader_filename<primitive_t>);

        return pipelineobjects;
    }

    template<typename geometry_t, gfx::topology primitive_t>
    inline std::vector<ComPtr<ID3D12Resource>> body_dynamic<geometry_t, primitive_t>::create_resources()
    {
        m_vertices = get_vertices(body);
        assert(m_vertices.size() < AS_GROUP_SIZE * MAX_MSGROUPS_PER_ASGROUP * MAX_TRIANGLES_PER_GROUP * 3);

        m_vertexbuffer = create_vertexbuffer_upload(&m_vertexbuffer_databegin, m_vertices.data(), get_vertexbuffersize());
        return std::vector<ComPtr<ID3D12Resource>>();
    }

    template<typename geometry_t, typename = std::enable_if_t<std::is_lvalue_reference_v<geometry_t>>>
    inline void update_body(std::type_identity_t<std::decay_t<geometry_t>> const& body, float dt)
    {
    }

    template<typename geometry_t>
    inline void update_body(std::type_identity_t<std::decay_t<geometry_t>> &body, float dt) { body.update(dt); }

    template<typename geometry_t, gfx::topology primitive_t>
    inline void body_dynamic<geometry_t, primitive_t>::update(float dt)
    {
        // todo : find a better way to organize bits only used by value/reference type geometry_t
        update_body<geometry_t>(body, dt);
        m_vertices = get_vertices(body);
        assert(m_vertices.size() < AS_GROUP_SIZE * MAX_MSGROUPS_PER_ASGROUP * MAX_TRIANGLES_PER_GROUP * 3);
    }

    template<typename geometry_t, gfx::topology primitive_t>
    inline void body_dynamic<geometry_t, primitive_t>::update_vertexbuffer()
    {
        update_perframebuffer(m_vertexbuffer_databegin, m_vertices.data(), get_vertexbuffersize());
    }

    template<typename geometry_t, gfx::topology primitive_t>
    inline D3D12_GPU_VIRTUAL_ADDRESS body_dynamic<geometry_t, primitive_t>::get_vertexbuffer_gpuaddress() const
    {
        return get_perframe_gpuaddress(m_vertexbuffer->GetGPUVirtualAddress(), get_vertexbuffersize());
    }
}