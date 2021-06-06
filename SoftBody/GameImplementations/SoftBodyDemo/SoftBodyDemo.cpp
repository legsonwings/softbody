#include "stdafx.h"
#include "SoftBodyDemo.h"
#include "GameUtils.h"
#include "SharedConstants.h"
#include "Engine/interfaces/engineinterface.h"
#include "Engine/DXSample.h"
#include "Engine/engineutils.h"
#include "Engine/graphics/body.h"

#include <random>
#include <algorithm>
#include <functional>

namespace game_creator
{
	template <>
	std::unique_ptr<game_base> create_instance<game_types::soft_body_demo>(game_engine const* engine)
	{
		return std::move(std::make_unique<soft_body>(engine));
	}
}

using namespace DirectX;
using namespace DirectX::SimpleMath;

std::random_device rd;
std::mt19937 mt(rd());

soft_body::soft_body(game_engine const* engine)
	: game_base(engine)
{
    assert(engine != nullptr);

    m_camera.Init({ 0, 0, 10.f });
    m_camera.SetMoveSpeed(10.0f);

    uintptr_t const temp = reinterpret_cast<uintptr_t>(constant_buffer_memory) + cb_alignment - 1;
    m_constantbuffer_data = reinterpret_cast<SceneConstantBuffer*>(temp & ~static_cast<uintptr_t>(cb_alignment - 1));
}

void soft_body::update(float dt)
{
    m_camera.Update(dt);

    static std::size_t const num_dynamicbodies = dynamicbodies_tri.size();
    for (std::size_t i = 0; i < num_dynamicbodies; ++i)
    {
        for (std::size_t j = i + 1; j < num_dynamicbodies; ++j)
        {
            dynamicbodies_tri[i].get().resolve_collision(dynamicbodies_tri[j].get(), dt);
        }
    }

    for (auto& body : dynamicbodies_tri) { body.update(dt); }
    for (auto& body : dynamicbodies_line) { body.update(dt); }

    XMMATRIX world = XMMATRIX(g_XMIdentityR0, g_XMIdentityR1, g_XMIdentityR2, g_XMIdentityR3);
    XMMATRIX view = m_camera.GetViewMatrix();
    XMMATRIX proj = m_camera.GetProjectionMatrix(XM_PI / 3.0f, engine->get_config_properties().get_aspect_ratio());

    XMFLOAT3 cameraPos = m_camera.GetCurrentPosition();
    XMStoreFloat3(&(m_constantbuffer_data->CamPos), XMLoadFloat3(&cameraPos));
    XMStoreFloat4x4(&(m_constantbuffer_data->World), XMMatrixTranspose(world));
    XMStoreFloat4x4(&(m_constantbuffer_data->WorldView), XMMatrixTranspose(world * view));
    XMStoreFloat4x4(&(m_constantbuffer_data->WorldViewProj), XMMatrixTranspose(world * view * proj));

    memcpy(m_cbv_databegin + sizeof(SceneConstantBuffer) * engine->get_frame_index(), m_constantbuffer_data, sizeof(SceneConstantBuffer));
}

void soft_body::render(float dt)
{
    dispatch_bodies(dynamicbodies_tri);

    if (m_debugviz_toggle)
    {
        dispatch_bodies(dynamicbodies_line);
        dispatch_bodies(staticbodies_lines);
    }
}

game_base::resourcelist soft_body::load_assets_and_geometry()
{
    auto device = engine->get_device();

    m_constantbuffer = create_upload_buffer(&m_cbv_databegin, sizeof(SceneConstantBuffer) * configurable_properties::frame_count);

    //static const std::uniform_real_distribution<float> color_dist(0.f, 1.f);
    //Vector3 const color = { color_dist(mt), color_dist(mt), color_dist(mt) };

    Vector3 const lposition = { -5.f, 0.f, 0.f };
    Vector3 const rposition = {5.f, 0.f, 0.f};

    using geometry::ffd_object;
    dynamicbodies_tri.emplace_back(ffd_object{ { lposition, 1.f } });
    dynamicbodies_tri.emplace_back(ffd_object{ { rposition, 1.f } });

    dynamicbodies_line.emplace_back(*dynamicbodies_tri[0], &ffd_object::getbox_vertices);
    dynamicbodies_line.emplace_back(*dynamicbodies_tri[1], &ffd_object::getbox_vertices);
    staticbodies_lines.emplace_back(*dynamicbodies_tri[0], &ffd_object::get_control_point_visualization, &ffd_object::get_controlnet_instancedata);
    staticbodies_lines.emplace_back(*dynamicbodies_tri[1], &ffd_object::get_control_point_visualization, &ffd_object::get_controlnet_instancedata);

    dynamicbodies_tri[0].get().set_velocity({ 8.f, 0.f, 0.f });
    dynamicbodies_tri[1].get().set_velocity({ -8.f, 0.f, 0.f });

    game_base::resourcelist retvalues;
    retvalues.reserve(dynamicbodies_tri.size() + staticbodies_lines.size());

    // todo : provide helpers to append
    for (auto &b : dynamicbodies_tri) 
    { 
        auto const& body_res = b.create_resources();
        retvalues.insert(retvalues.end(), body_res.begin(), body_res.end());
    };

    for (auto& b : dynamicbodies_line)
    {
        auto const& body_res = b.create_resources();
        retvalues.insert(retvalues.end(), body_res.begin(), body_res.end());
    };
    
    for (auto &b : staticbodies_lines) 
    {
        auto const& body_res = b.create_resources();
        retvalues.insert(retvalues.end(), body_res.begin(), body_res.end());
    };

    return retvalues;
}

void soft_body::switch_cameraview()
{
    switch (camera_view)
    {
    case 1:
    {
        m_camera.TopView();
        break;
    }
    case 2:
    {
        m_camera.BotView();
        break;
    }
    }
}

void soft_body::on_key_down(unsigned key)
{
    m_camera.OnKeyDown(key);

    if (key == 'T')
    {
        m_wireframe_toggle = !m_wireframe_toggle;
    }

    if (key == 'v')
    {
        m_debugviz_toggle = !m_debugviz_toggle;
    }

    if (key == 'L')
    {
        toggle1 = !toggle1;
    }

    if (key == 'R')
    {
        toggle2 = !toggle2;
    }

    unsigned const relative_key = key - '0';
    if (relative_key > 0 && relative_key < 5)
    {
        camera_view = relative_key;
        switch_cameraview();
    }
    else if(camera_view != 0)
    {
        camera_view = 0;
    }
}

void soft_body::on_key_up(unsigned key)
{
    m_camera.OnKeyUp(key);
}

ComPtr<ID3D12Resource> soft_body::create_upload_buffer(uint8_t** mapped_buffer, size_t const buffer_size) const
{
    auto device = engine->get_device();

    ComPtr<ID3D12Resource> upload_buffer;
    auto heap_props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto resource_desc = CD3DX12_RESOURCE_DESC::Buffer(buffer_size);
    ThrowIfFailed(device->CreateCommittedResource(&heap_props, D3D12_HEAP_FLAG_NONE, &resource_desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(upload_buffer.ReleaseAndGetAddressOf())));
    ThrowIfFailed(upload_buffer->Map(0, nullptr, reinterpret_cast<void**>(mapped_buffer)));

    return upload_buffer;
}

template<typename geometry_t, gfx::topology primitive_t>
typename std::enable_if_t<primitive_t == gfx::topology::triangle, void> soft_body::dispatch_bodies(std::vector<gfx::body_dynamic<geometry_t>> &bodies)
{
    auto frame_idx = engine->get_frame_index();
    auto cmd_list = engine->get_command_list();
    
    D3D12_GPU_VIRTUAL_ADDRESS const cb_gpuaddress = m_constantbuffer->GetGPUVirtualAddress() + sizeof(SceneConstantBuffer) * frame_idx;

    gfx::pipeline_objects const& pipelineobjects = gfx::body_dynamic<geometry_t>::get_static_pipelineobjects();
    cmd_list->SetPipelineState((m_wireframe_toggle && pipelineobjects.pso_wireframe) ? pipelineobjects.pso_wireframe.Get() : pipelineobjects.pso.Get());
    cmd_list->SetGraphicsRootSignature(pipelineobjects.root_signature.Get());
    cmd_list->SetGraphicsRootConstantBufferView(0, cb_gpuaddress);

    // todo : debug code
    int i = 0;
    for (auto & body : bodies)
    {
        ++i;

        if ((i == 1 && toggle1) || (i == 2 && toggle2))
            continue;

        struct dispatch_parameters
        {
            uint32_t num_triangles;
            Vector3 color = { 1.f, 0.3f, 0.1f };
        } dispatch_params;

        body.update_vertexbuffer();
        dispatch_params.num_triangles = body.get_numvertices() / 3;

        // Draw the spheres
        cmd_list->SetGraphicsRoot32BitConstants(1, 4, &dispatch_params, 0);

        // these shouldn't be directly int the root signature, use descriptor heap
        cmd_list->SetGraphicsRootShaderResourceView(2, body.get_vertexbuffer_gpuaddress());

        cmd_list->DispatchMesh(1, 1, 1);
    }
}

template<typename geometry_t, gfx::topology primitive_t>
typename std::enable_if_t < primitive_t == gfx::topology::line , void> soft_body::dispatch_bodies(std::vector<gfx::body_dynamic<geometry_t, primitive_t>> &bodies)
{
    auto frame_idx = engine->get_frame_index();
    auto cmd_list = engine->get_command_list();

    D3D12_GPU_VIRTUAL_ADDRESS const cb_gpuaddress = m_constantbuffer->GetGPUVirtualAddress() + sizeof(SceneConstantBuffer) * frame_idx;

    gfx::pipeline_objects const& pipelineobjects = gfx::body_dynamic<geometry_t, primitive_t>::get_static_pipelineobjects();
    cmd_list->SetPipelineState((m_wireframe_toggle && pipelineobjects.pso_wireframe) ? pipelineobjects.pso_wireframe.Get() : pipelineobjects.pso.Get());
    cmd_list->SetGraphicsRootSignature(pipelineobjects.root_signature.Get());
    cmd_list->SetGraphicsRootConstantBufferView(0, cb_gpuaddress);

    for (auto &body : bodies)
    {
        struct dispatch_params
        {
            uint32_t num_prims;
            float color[3] = { 1.f, 1.f, 0.f};
        };

        body.update_vertexbuffer();

        dispatch_params info;
        info.num_prims = static_cast<unsigned>(body.get_numvertices() / 2);

        cmd_list->SetGraphicsRoot32BitConstants(1, 4, &info, 0);
        cmd_list->SetGraphicsRootShaderResourceView(3, body.get_vertexbuffer_gpuaddress());

        unsigned const num_lines = info.num_prims;
        unsigned const num_ms = (num_lines / MAX_LINES_PER_MS) + ((num_lines % MAX_LINES_PER_MS) == 0 ? 0 : 1);

        unsigned num_lines_processed = 0;
        for (unsigned ms_idx = 0; ms_idx < num_ms; ++ms_idx)
        {
            cmd_list->SetGraphicsRoot32BitConstant(2, num_lines_processed, 0);
            cmd_list->DispatchMesh(1, 1, 1);

            num_lines_processed += MAX_LINES_PER_MS;
        }
    }
}

template<typename geometry_t, gfx::topology primitive_t>
typename std::enable_if_t<primitive_t == gfx::topology::triangle, void> soft_body::dispatch_bodies(std::vector<gfx::body_static<geometry_t>> &bodies)
{
    auto frame_idx = engine->get_frame_index();
    auto cmd_list = engine->get_command_list();

    D3D12_GPU_VIRTUAL_ADDRESS const cb_gpuaddress = m_constantbuffer->GetGPUVirtualAddress() + sizeof(SceneConstantBuffer) * frame_idx;

    struct dispatch_parameters
    {
        uint32_t num_tris;
        uint32_t tris_perinstance;
    };

    gfx::pipeline_objects const& pipelineobjects = gfx::body_static<geometry::ffd_object const&, primitive_t>::get_static_pipelineobjects();

    cmd_list->SetPipelineState((m_wireframe_toggle&& pipelineobjects.pso_wireframe) ? pipelineobjects.pso_wireframe.Get() : pipelineobjects.pso.Get());
    cmd_list->SetGraphicsRootSignature(pipelineobjects.root_signature.Get());
    cmd_list->SetGraphicsRootConstantBufferView(0, cb_gpuaddress);

    for (auto& body : bodies)
    {
        body.update_instancebuffer();

        dispatch_parameters info;
        info.tris_perinstance = body.get_numvertices() / 3;
        info.num_tris = body.get_numinstances() * info.tris_perinstance;

        cmd_list->SetGraphicsRoot32BitConstants(1, 2, &info, 0);
        cmd_list->SetGraphicsRootShaderResourceView(2, body.get_vertexbuffer_gpuaddress());
        cmd_list->SetGraphicsRootShaderResourceView(3, body.get_instancebuffer_gpuaddress());

        cmd_list->DispatchMesh(1, 1, 1);
    }
}

template<typename geometry_t, gfx::topology primitive_t>
typename std::enable_if_t<primitive_t == gfx::topology::line, void> soft_body::dispatch_bodies(std::vector<gfx::body_static<geometry_t, primitive_t>> &bodies)
{
    auto frame_idx = engine->get_frame_index();
    auto cmd_list = engine->get_command_list();

    D3D12_GPU_VIRTUAL_ADDRESS const cb_gpuaddress = m_constantbuffer->GetGPUVirtualAddress() + sizeof(SceneConstantBuffer) * frame_idx;

    struct instance_info
    {
        uint32_t num_instances;
        uint32_t num_primitives_per_instance;
    };

    gfx::pipeline_objects const& pipelineobjects = gfx::body_static<geometry::ffd_object const&, primitive_t>::get_static_pipelineobjects();

    cmd_list->SetPipelineState((m_wireframe_toggle && pipelineobjects.pso_wireframe) ? pipelineobjects.pso_wireframe.Get() : pipelineobjects.pso.Get());
    cmd_list->SetGraphicsRootSignature(pipelineobjects.root_signature.Get());
    cmd_list->SetGraphicsRootConstantBufferView(0, cb_gpuaddress);

    for (auto& body : bodies)
    {
        body.update_instancebuffer();

        instance_info info;
        info.num_instances = static_cast<unsigned>(body.get_numinstances());
        info.num_primitives_per_instance = static_cast<unsigned>(body.get_numvertices() / 2);

        cmd_list->SetGraphicsRoot32BitConstants(1, 2, &info, 0);
        cmd_list->SetGraphicsRootShaderResourceView(3, body.get_vertexbuffer_gpuaddress());
        cmd_list->SetGraphicsRootShaderResourceView(4, body.get_instancebuffer_gpuaddress());

        unsigned const num_lines = info.num_primitives_per_instance * info.num_instances;
        unsigned const num_ms = (num_lines / MAX_LINES_PER_MS) + ((num_lines % MAX_LINES_PER_MS) == 0 ? 0 : 1);

        unsigned num_lines_processed = 0;
        for (unsigned ms_idx = 0; ms_idx < num_ms; ++ms_idx)
        {
            cmd_list->SetGraphicsRoot32BitConstant(2, num_lines_processed, 0);
            cmd_list->DispatchMesh(1, 1, 1);

            num_lines_processed += MAX_LINES_PER_MS;
        }
    }
}