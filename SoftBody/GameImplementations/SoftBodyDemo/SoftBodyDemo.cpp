#include "stdafx.h"
#include "SoftBodyDemo.h"
#include "GameUtils.h"
#include "SharedConstants.h"
#include "Engine/Interfaces/GameEngineInterface.h"
#include "Engine/DXSample.h"
#include "Engine/EngineUtils.h"
#include "Engine/Graphics/Body.h"

#include <random>
#include <iterator>
#include <algorithm>

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

    m_cpu_instance_data_control_net.reset(new controlnet_instance_data[c_num_controlpoints]);
    uintptr_t const temp = reinterpret_cast<uintptr_t>(constant_buffer_memory) + cb_alignment - 1;
    m_constantbuffer_data = reinterpret_cast<SceneConstantBuffer*>(temp & ~static_cast<uintptr_t>(cb_alignment - 1));
}

void soft_body::update(float dt)
{
    m_camera.Update(dt);

    for (auto& body : spheres)
    {
        body->update(dt);
    }

    XMMATRIX world = XMMATRIX(g_XMIdentityR0, g_XMIdentityR1, g_XMIdentityR2, g_XMIdentityR3);
    XMMATRIX view = m_camera.GetViewMatrix();
    XMMATRIX proj = m_camera.GetProjectionMatrix(XM_PI / 3.0f, engine->get_config_properties().get_aspect_ratio());

    XMFLOAT3 cameraPos = m_camera.GetCurrentPosition();
    XMStoreFloat3(&(m_constantbuffer_data->CamPos), XMLoadFloat3(&cameraPos));
    XMStoreFloat4x4(&(m_constantbuffer_data->World), XMMatrixTranspose(world));
    XMStoreFloat4x4(&(m_constantbuffer_data->WorldView), XMMatrixTranspose(world * view));
    XMStoreFloat4x4(&(m_constantbuffer_data->WorldViewProj), XMMatrixTranspose(world * view * proj));

    memcpy(m_cbv_databegin + sizeof(SceneConstantBuffer) * engine->get_frame_index(), m_constantbuffer_data, sizeof(SceneConstantBuffer));

    auto frame_idx = engine->get_frame_index();
    memcpy(m_ibv_mapped_controlnet + sizeof(controlnet_instance_data) * c_num_controlpoints * frame_idx, m_cpu_instance_data_control_net.get(), sizeof(controlnet_instance_data) * c_num_controlpoints);
}

void soft_body::render(float dt)
{
}

void soft_body::populate_command_list()
{
    auto cmd_list = engine->get_command_list();
    auto frame_idx = engine->get_frame_index();

    D3D12_GPU_VIRTUAL_ADDRESS const cb_gpuaddress = m_constantbuffer->GetGPUVirtualAddress() + sizeof(SceneConstantBuffer) * frame_idx;

    {
        gfx::pipeline_objects const& pipelineobjects = gfx::body_dynamic<Geometry::ffd_object>::get_static_pipelineobjects();
        cmd_list->SetPipelineState((m_wireframe_toggle && pipelineobjects.pso_wireframe) ? pipelineobjects.pso_wireframe.Get() : pipelineobjects.pso.Get());
        cmd_list->SetGraphicsRootSignature(pipelineobjects.root_signature.Get());
        cmd_list->SetGraphicsRootConstantBufferView(0, cb_gpuaddress);
        for (auto& body : spheres)
        {
            struct dispatch_parameters
            {
                uint32_t num_triangles;
                Vector3 color = { 1.f, 0.3f, 0.1f };
            } dispatch_params;

            body->update_vertexbuffer();
            dispatch_params.num_triangles = body->get_numvertices() / 3;

            // Draw the spheres
            cmd_list->SetGraphicsRoot32BitConstants(1, 4, &dispatch_params, 0);

            // these shouldn't be directly int the root signature, use descriptor heap
            cmd_list->SetGraphicsRootShaderResourceView(2, body->get_vertexbuffer_gpuaddress());

            //cmd_list->DispatchMesh(1, 1, 1);
        }
    }
    {
        gfx::pipeline_objects const& pipelineobjects = gfx::body_static<Geometry::circle>::get_static_pipelineobjects();
        cmd_list->SetPipelineState((m_wireframe_toggle && pipelineobjects.pso_wireframe) ? pipelineobjects.pso_wireframe.Get() : pipelineobjects.pso.Get());
        cmd_list->SetGraphicsRootSignature(pipelineobjects.root_signature.Get());
        cmd_list->SetGraphicsRootConstantBufferView(0, cb_gpuaddress);

        circle->update_instancebuffer();
        unsigned const num_tris = circle->get_numvertices() / 3;

        // draw the circle
        cmd_list->SetGraphicsRoot32BitConstant(1, num_tris, 0);
        cmd_list->SetGraphicsRoot32BitConstant(1, num_tris, 1);
        
        // these shouldn't be directly int the root signature, use descriptor heap
        cmd_list->SetGraphicsRootShaderResourceView(2, circle->get_vertexbuffer_gpuaddress());
        cmd_list->SetGraphicsRootShaderResourceView(3, circle->get_instancebuffer_gpuaddress());

        //cmd_list->DispatchMesh(1, 1, 1);
    }

    {
        struct
        {
            float color[4] = { 1.f, 1.f, 0.f, 1.f };
            uint32_t num_instances;
            uint32_t num_primitives_per_instance;
        } controlnet_info;

        controlnet_info.num_instances = c_num_controlpoints;
        controlnet_info.num_primitives_per_instance = static_cast<unsigned>(m_vertices_controlnet.size() / 2);

        // Draw the control net
        cmd_list->SetPipelineState(m_pipelinestate_lines.Get());
        cmd_list->SetGraphicsRootSignature(m_rootsignature_lines.Get());
        cmd_list->SetGraphicsRootConstantBufferView(0, cb_gpuaddress);
        cmd_list->SetGraphicsRoot32BitConstants(1, 6, &controlnet_info, 0);
        cmd_list->SetGraphicsRootShaderResourceView(3, m_vertexbuffer_controlnet->GetGPUVirtualAddress());
        cmd_list->SetGraphicsRootShaderResourceView(4, m_instance_buffer_controlnet->GetGPUVirtualAddress() + sizeof(controlnet_instance_data) * c_num_controlpoints * frame_idx);

        // TODO : Move this to shared constants
        static constexpr unsigned max_lines_per_ms = 64;
        unsigned const num_lines = controlnet_info.num_primitives_per_instance * controlnet_info.num_instances;
        
        unsigned const num_ms = (num_lines / max_lines_per_ms) + ((num_lines % max_lines_per_ms) == 0 ? 0 : 1);

        unsigned num_lines_processed = 0;
        for (unsigned ms_idx = 0; ms_idx < num_ms; ++ms_idx)
        {
            cmd_list->SetGraphicsRoot32BitConstant(2, num_lines_processed, 0);
            cmd_list->DispatchMesh(1, 1, 1);

            num_lines_processed += max_lines_per_ms;
        }
    }
}

std::vector<std::weak_ptr<gfx::body>> soft_body::load_assets_and_geometry()
{
    std::vector<std::weak_ptr<gfx::body>> return_bodies;

    auto device = engine->get_device();

    m_constantbuffer = create_upload_buffer(&m_cbv_databegin, sizeof(SceneConstantBuffer) * configurable_properties::frame_count);
    m_instance_buffer_controlnet = create_upload_buffer(&m_ibv_mapped_controlnet, c_num_controlpoints * configurable_properties::frame_count * sizeof(controlnet_instance_data));

    // Create the pipeline state, which includes compiling and loading shaders.
    {
        struct shader
        {
            byte* data;
            uint32_t size;
        };

        {
            shader meshshader_lines, pixelshader_lines;

            ReadDataFromFile(engine->get_asset_fullpath(c_lines_meshshader_filename).c_str(), &meshshader_lines.data, &meshshader_lines.size);
            ReadDataFromFile(engine->get_asset_fullpath(c_lines_pixelshader_filename).c_str(), &pixelshader_lines.data, &pixelshader_lines.size);

            ThrowIfFailed(device->CreateRootSignature(0, meshshader_lines.data, meshshader_lines.size, IID_PPV_ARGS(&m_rootsignature_lines)));

            D3DX12_MESH_SHADER_PIPELINE_STATE_DESC pso_desc_lines = engine->get_pso_desc();
            pso_desc_lines.pRootSignature = m_rootsignature_lines.Get();
            pso_desc_lines.MS = { meshshader_lines.data, meshshader_lines.size };
            pso_desc_lines.PS = { pixelshader_lines.data, pixelshader_lines.size };
            pso_desc_lines.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;

            auto psostream_lines = CD3DX12_PIPELINE_MESH_STATE_STREAM(pso_desc_lines);

            D3D12_PIPELINE_STATE_STREAM_DESC stream_desc_lines;
            stream_desc_lines.pPipelineStateSubobjectStream = &psostream_lines;
            stream_desc_lines.SizeInBytes = sizeof(psostream_lines);

            ThrowIfFailed(device->CreatePipelineState(&stream_desc_lines, IID_PPV_ARGS(m_pipelinestate_lines.ReleaseAndGetAddressOf())));
        }
    }

    // Load geometry
    static const float blob_size = 10.f;
    static const std::uniform_real_distribution<float> point_dist(-blob_size, blob_size + 1);
    static const std::uniform_real_distribution<float> color_dist(0.f, 1.f);

    Vector3 const color = { color_dist(mt), color_dist(mt), color_dist(mt) };
    Vector3 const position = Vector3::Zero;

    Geometry::ffd_object ffd_sphere = { {position} };
    ffd_sphere.apply_force(1, 2, 1, { 0.f, 2.f, 0.f });
    ffd_sphere.apply_force(2, 1, 1, { 5.f, 0.f, 0.f });

    spheres.push_back(std::make_shared<gfx::body_dynamic<Geometry::ffd_object>>(ffd_sphere));
    return_bodies.push_back(spheres.back());

    // test
    Geometry::circle localcircle;
    localcircle.center = { 3, 0, 0 };
    localcircle.radius = 1;
    localcircle.normal = { 1, 0, 0 };

    circle = std::make_shared<gfx::body_static<Geometry::circle>>(localcircle, 1);
    return_bodies.push_back(circle);


    {
        m_vertices_controlnet = ffd_sphere.get_control_point_visualization();

        if (m_vertices_controlnet.size() > 0)
        {
            auto vb_size = m_vertices_controlnet.size() * sizeof(Geometry::Vector3);
            auto vb_desc = CD3DX12_RESOURCE_DESC::Buffer(vb_size);

            // Create vertex buffer on the default heap
            auto defaultheap_desc = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
            ThrowIfFailed(device->CreateCommittedResource(&defaultheap_desc, D3D12_HEAP_FLAG_NONE, &vb_desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(m_vertexbuffer_controlnet.ReleaseAndGetAddressOf())));

            // Create vertex resource on the upload heap
            auto uploadheap_desc = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
            ThrowIfFailed(device->CreateCommittedResource(&uploadheap_desc, D3D12_HEAP_FLAG_NONE, &vb_desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(m_uploadbuffer_controlnet.GetAddressOf())));

            {
                uint8_t* vb_upload_start = nullptr;

                // We do not intend to read from this resource on the CPU.
                m_uploadbuffer_controlnet->Map(0, nullptr, reinterpret_cast<void**>(&vb_upload_start));

                // Copy vertex data to upload heap
                memcpy(vb_upload_start, m_vertices_controlnet.data(), vb_size);

                m_uploadbuffer_controlnet->Unmap(0, nullptr);
            }

            auto resource_transition = CD3DX12_RESOURCE_BARRIER::Transition(m_vertexbuffer_controlnet.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

            auto cmd_list = game_engine::g_engine->get_command_list();

            // Copy vertex data from upload heap to default heap
            cmd_list->CopyResource(m_vertexbuffer_controlnet.Get(), m_uploadbuffer_controlnet.Get());
            cmd_list->ResourceBarrier(1, &resource_transition);
        }
    }

    auto const& control_net = ffd_sphere.get_control_net();
    for (size_t ctrl_pt_idx = 0; ctrl_pt_idx < control_net.size(); ++ctrl_pt_idx)
    {
        m_cpu_instance_data_control_net[ctrl_pt_idx].position = control_net[ctrl_pt_idx];
    }

    return return_bodies;
}

void soft_body::on_key_down(unsigned key)
{
    m_camera.OnKeyDown(key);

    if (key == 'T')
    {
        m_wireframe_toggle = !m_wireframe_toggle;
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

//void SoftBody::UpdatePhysics()
//{
//    float const dt = static_cast<float>(m_timer.GetElapsedSeconds());

    //auto const omega = 3.f; //std::sqrt(spring.k / blob_physx[0].mass);
    //auto const eta = 0.5f;

    //auto const alpha = omega * std::sqrt(1 - eta * eta);
    //auto displacement = blob_physx[0].position - blob_physx[1].position;

    //// Simple harmonic motion(SHM)
    ////auto pos = displacement * std::cos(omega * dt) + (blob_physx[0].velocity * std::sin(omega * dt)) / omega;
    ////blob_physx[0].velocity = displacement * (-omega) * std::sin(omega * dt) + blob_physx[0].velocity * std::cos(omega * dt);

    //// Damped SHM
    //auto pos = std::powf(2.71828f, -omega * eta * dt) * (displacement * std::cos(alpha * dt) + (blob_physx[0].velocity + omega * eta * displacement) * std::sin(alpha * dt) / alpha);
    //blob_physx[0].velocity = -std::pow(2.71828f, -omega * eta * dt) * ((displacement * omega * eta - blob_physx[0].velocity - omega * eta * displacement) * cos(alpha * dt) +
    //    (displacement * alpha + (blob_physx[0].velocity + omega * eta * displacement) * omega * eta / alpha) * sin(alpha * dt));

    //blob_physx[0].position = blob_physx[1].position + pos;

    //m_cpu_instance_data[0].Position = Geometry::Utils::create_Vector4(blob_physx[0].position);

    //auto const omega = 1.f; //std::sqrt(spring.k / blob_physx[0].mass);
    //auto const eta = 0.5f;
    //auto const alpha = omega * std::sqrt(1 - eta * eta);

    //for (auto& spring : springs)
    //{
    //    //blob_physx[spring.m1].new_position = blob_physx[spring.m1].position;
    //    //blob_physx[spring.m2].new_position = blob_physx[spring.m2].position;

    //    auto dir_m2 = (blob_physx[spring.m2].position - blob_physx[spring.m1].position);
    //    spring.l = dir_m2.Length();
    //    dir_m2.Normalize();

    //    auto mean_pos = (blob_physx[spring.m2].position + blob_physx[spring.m1].position) * 0.5f;
    //    float rest_len = 2.f;
    //    auto stretch = (spring.l - rest_len);

    //    // This is underdamped spring
    //    // Try critically damped or overdamped
    //    float const new_stretch = std::powf(2.71828f, -omega * eta * dt) * (stretch * std::cos(alpha * dt) + (spring.v + omega * eta * stretch) * std::sinf(alpha * dt) / alpha);
    //    spring.v = -std::pow(2.71828f, -omega * eta * dt) * ((stretch * omega * eta - spring.v - omega * eta * stretch) * cos(alpha * dt) +
    //        (stretch * alpha + (spring.v + omega * eta * stretch) * omega * eta / alpha) * sin(alpha * dt));

    //    // This is to ensure that the spring length is never negative
    //    // Ideally we would want to apply resisting force to the spring masses if they are coming too close
    //    spring.l = std::max(0.3f, new_stretch + rest_len);

    //    blob_physx[spring.m1].new_position += mean_pos + (spring.l * 0.5f * -dir_m2) - blob_physx[spring.m1].position;
    //    blob_physx[spring.m2].new_position += mean_pos + (spring.l * 0.5f * dir_m2) - blob_physx[spring.m2].position;
//
//#if CONSOLE_LOGS
//        if (m_timer.GetFrameCount() < 2)
//        {
//            std::cout << "\nSprings : [" << spring.m1 << ", " << spring.m2 << "]. " << "Pos : [" << blob_physx[spring.m1].new_position.x << ", "
//                << blob_physx[spring.m1].new_position.y << ", " << blob_physx[spring.m1].new_position.z << ", "
//                << blob_physx[spring.m2].new_position.x << ", " << blob_physx[spring.m2].new_position.y << ", " << blob_physx[spring.m2].new_position.z << "]. ";
//        }
//#endif
//
//    }

    //for (unsigned i = 0; i < num_spheres; ++i)
    //    for (unsigned j = i + 1; j < num_spheres; ++j)
    //    {
    //        blob_physx[i].new_position = blob_physx[connecting_spring.m1].position;
    //        blob_physx[j].new_position = blob_physx[connecting_spring.m2].position;

    //        // The first connecting spring for ith spring is given by i (n - (i + 1)/2)
    //        unsigned const spring_idx = (2 * num_spheres - i - 1) * 0.5f * i + j - i - 1;
    //        Geometry::Spring & connecting_spring = springs[spring_idx];
    //        auto dir_m2 = (blob_physx[connecting_spring.m2].position - blob_physx[connecting_spring.m1].position);
    //        connecting_spring.l = dir_m2.Length();
    //        dir_m2.Normalize();

    //        auto mean_pos = (blob_physx[j].position + blob_physx[i].position) * 0.5f;
    //        float rest_len = 2.f;
    //        auto stretch = (connecting_spring.l - rest_len);

    //        float const new_stretch =  std::powf(2.71828f, -omega * eta * dt) * (stretch * std::cos(alpha * dt) + (connecting_spring.v + omega * eta * stretch) * std::sinf(alpha * dt) / alpha);
    //        connecting_spring.v = -std::pow(2.71828f, -omega * eta * dt) * ((stretch * omega * eta - connecting_spring.v - omega * eta * stretch) * cos(alpha * dt) +
    //                                (stretch * alpha + (connecting_spring.v + omega * eta * stretch) * omega * eta / alpha) * sin(alpha * dt));
    //        connecting_spring.l = std::max(0.3f, new_stretch + rest_len);

    //        blob_physx[i].new_position += mean_pos + (connecting_spring.l * 0.5f * -dir_m2) - blob_physx[i].position;
    //        blob_physx[j].new_position += mean_pos + (connecting_spring.l * 0.5f * dir_m2) - blob_physx[j].position;
    //    }

    //for (unsigned i = 0; i < num_spheres; ++i)
    //{
    //    /*Vector3 const acc = blob_physx[i].force / blob_physx[i].mass;
    //    blob_physx[i].velocity += acc * dt;
    //    blob_physx[i].position += blob_physx[i].velocity * dt;*/

    //    blob_physx[i].position = blob_physx[i].new_position;
    //    m_cpu_instance_data[i].Position = { blob_physx[i].position.x, blob_physx[i].position.y, blob_physx[i].position.z, 0.f };
    //}

    //for(unsigned i = 0; i < num_spheres; ++i)
    //    for (unsigned j = i + 1; j < num_spheres; ++j)
    //    {
    //        // The first connecting spring for ith spring is given by i (n - (i + 1)/2)
    //        unsigned const spring_idx = (2 * num_spheres - i - 1) * 0.5f * i + j - i - 1;
    //        Geometry::Spring const &connecting_spring = springs[spring_idx];

    //        auto displacement = blob_physx[j].position - blob_physx[i].position;
    //        float const dist = displacement.Length();
    //        displacement.Normalize();

    //        float const stretch = dist - connecting_spring.l;
    //        auto dvelocity = blob_physx[j].velocity - blob_physx[i].velocity;

    //        float const force = (stretch * connecting_spring.k);
    //        blob_physx[i].force += displacement * force;
    //        blob_physx[j].force -= displacement * force;
    //    }

    //for (unsigned i = 0; i < num_spheres; ++i)
    //{
    //    Vector3 const acc = blob_physx[i].force / blob_physx[i].mass;
    //    blob_physx[i].velocity += acc * dt;
    //    blob_physx[i].position += blob_physx[i].velocity * dt;
    //    m_cpu_instance_data[i].Position = blob_physx[i].position;
    //}
//}