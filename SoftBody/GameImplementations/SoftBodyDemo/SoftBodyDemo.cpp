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

    uintptr_t const temp = reinterpret_cast<uintptr_t>(constant_buffer_memory) + cb_alignment - 1;
    m_constantbuffer_data = reinterpret_cast<SceneConstantBuffer*>(temp & ~static_cast<uintptr_t>(cb_alignment - 1));
}

void soft_body::update(float dt)
{
    m_camera.Update(dt);

    static std::size_t const num_spheres = spheres.size();
    for (std::size_t i = 0; i < num_spheres; ++i)
    {
        for (std::size_t j = i + 1; j < num_spheres; ++j)
        {
            spheres[i]->get_body().resolve_collision(spheres[j]->get_body(), dt);
        }
    }

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

        // todo : debug code
        int i = 0;
        for (auto& body : spheres)
        {
            ++i;
            
            if ((i == 1 && toggle1) || (i == 2 && toggle2))
                continue;

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

            cmd_list->DispatchMesh(1, 1, 1);
        }
    }

    //{
    //    struct
    //    {
    //        float color[4] = { 1.f, 1.f, 0.f, 1.f };
    //        uint32_t num_instances;
    //        uint32_t num_primitives_per_instance;
    //    } sphere_isectinfo;

    //    sphere_isectinfo.num_instances = sphere_isect->get_numinstances();
    //    sphere_isectinfo.num_primitives_per_instance = static_cast<unsigned>(sphere_isect->get_numvertices() / 2);

    //    gfx::pipeline_objects const& pipelineobjects = gfx::body_static<Geometry::nullshape, gfx::topology::line>::get_static_pipelineobjects();

    //    cmd_list->SetPipelineState((m_wireframe_toggle && pipelineobjects.pso_wireframe) ? pipelineobjects.pso_wireframe.Get() : pipelineobjects.pso.Get());
    //    cmd_list->SetGraphicsRootSignature(pipelineobjects.root_signature.Get());
    //    cmd_list->SetGraphicsRootConstantBufferView(0, cb_gpuaddress);
    //    cmd_list->SetGraphicsRoot32BitConstants(1, 6, &sphere_isectinfo, 0);
    //    cmd_list->SetGraphicsRootShaderResourceView(3, sphere_isect->get_vertexbuffer_gpuaddress());
    //    cmd_list->SetGraphicsRootShaderResourceView(4, sphere_isect->get_instancebuffer_gpuaddress());

    //    unsigned const num_lines = sphere_isectinfo.num_primitives_per_instance * sphere_isectinfo.num_instances;

    //    unsigned const num_ms = (num_lines / MAX_LINES_PER_MS) + ((num_lines % MAX_LINES_PER_MS) == 0 ? 0 : 1);

    //    unsigned num_lines_processed = 0;
    //    for (unsigned ms_idx = 0; ms_idx < num_ms; ++ms_idx)
    //    {
    //        cmd_list->SetGraphicsRoot32BitConstant(2, num_lines_processed, 0);
    //        cmd_list->DispatchMesh(1, 1, 1);

    //        num_lines_processed += MAX_LINES_PER_MS;
    //    }
    //}

    {
        struct controlnet_info
        {
            float color[4] = { 1.f, 1.f, 0.f, 1.f };
            uint32_t num_instances;
            uint32_t num_primitives_per_instance;
        };

        gfx::pipeline_objects const& pipelineobjects = gfx::body_static<Geometry::ffd_object const&, gfx::topology::line>::get_static_pipelineobjects();

        cmd_list->SetPipelineState((m_wireframe_toggle&& pipelineobjects.pso_wireframe) ? pipelineobjects.pso_wireframe.Get() : pipelineobjects.pso.Get());
        cmd_list->SetGraphicsRootSignature(pipelineobjects.root_signature.Get());
        cmd_list->SetGraphicsRootConstantBufferView(0, cb_gpuaddress);

        for (auto& control_net : controlnets)
        {
            control_net->update_instancebuffer();

            controlnet_info info;
            info.num_instances = control_net->get_numinstances();
            info.num_primitives_per_instance = static_cast<unsigned>(control_net->get_numvertices() / 2);

            cmd_list->SetGraphicsRoot32BitConstants(1, 6, &info, 0);
            cmd_list->SetGraphicsRootShaderResourceView(3, control_net->get_vertexbuffer_gpuaddress());
            cmd_list->SetGraphicsRootShaderResourceView(4, control_net->get_instancebuffer_gpuaddress());

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
}

std::vector<std::weak_ptr<gfx::body>> soft_body::load_assets_and_geometry()
{
    auto device = engine->get_device();

    m_constantbuffer = create_upload_buffer(&m_cbv_databegin, sizeof(SceneConstantBuffer) * configurable_properties::frame_count);

    // Load geometry
    static const float blob_size = 10.f;
    static const std::uniform_real_distribution<float> point_dist(-blob_size, blob_size + 1);
    static const std::uniform_real_distribution<float> color_dist(0.f, 1.f);

    Vector3 const color = { color_dist(mt), color_dist(mt), color_dist(mt) };
    Vector3 const lposition = { -7.f, 0.f, 0.f };
    Vector3 const rposition = {7.f, 0.f, 0.f};

    Geometry::ffd_object l = { {lposition, 1.f} };
    Geometry::ffd_object r = { {rposition, 1.f} };

    spheres.push_back(std::make_shared<gfx::body_dynamic<Geometry::ffd_object>>(l));
    spheres.push_back(std::make_shared<gfx::body_dynamic<Geometry::ffd_object>>(r));

    spheres[0]->get_body().set_velocity({ 8.f, 0.f, 0.f });
    spheres[1]->get_body().set_velocity({ -8.f, 0.f, 0.f });

    //{ 
    //    // Todo : create a template to work without all type specifications
    //    sphere_isect = std::make_shared<gfx::body_static<Geometry::nullshape, gfx::topology::line>>(Geometry::nullshape{},
    //        [ls = spheres[0], rs = spheres[1]](Geometry::nullshape const&) { return utils::flatten(ls->get_body().intersect(rs->get_body())); },
    //            [](Geometry::nullshape const&) { return std::vector<gfx::instance_data>{ {Vector3::Zero, { 1.f, 0.f, 0.f } } }; });
    //}

    // todo : support for instances with single color for all vertices
    auto to_instancedata = [](std::vector<Vector3> const& locations, Vector3 const &color)
    {
        std::vector<gfx::instance_data> instances;
        instances.resize(locations.size());
        std::transform(locations.begin(), locations.end(), instances.begin(), [&color](Vector3 const& v) { return gfx::instance_data{ v, color }; });
        return instances;
    };

    controlnets.push_back(std::make_shared<gfx::body_static<Geometry::ffd_object const&, gfx::topology::line>>(spheres[0]->get_body(),
                                    [](Geometry::ffd_object const& ffd_obj) { return ffd_obj.get_control_point_visualization(); },
                                    [to_instancedata](Geometry::ffd_object const& ffd_obj) { return to_instancedata(ffd_obj.get_control_net(), { 1.f, 0.f, 0.f }); }));


    controlnets.push_back(std::make_shared<gfx::body_static<Geometry::ffd_object const&, gfx::topology::line>>(spheres[1]->get_body(),
                                    [](Geometry::ffd_object const& ffd_obj) { return ffd_obj.get_control_point_visualization(); },
                                    [to_instancedata](Geometry::ffd_object const& ffd_obj) { return to_instancedata(ffd_obj.get_control_net(), { 1.f, 0.f, 0.f }); }));

    return { spheres[0], spheres[1], controlnets[0], controlnets[1] };
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

    //m_cpu_instance_data[0].Position = utils::create_Vector4(blob_physx[0].position);

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