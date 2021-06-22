#include "stdafx.h"
#include "softbodydemo.h"
#include "gameutils.h"
#include "sharedconstants.h"
#include "engine/interfaces/engineinterface.h"
#include "engine/DXSample.h"
#include "engine/engineutils.h"
#include "engine/graphics/body.h"
#include "engine/graphics/gfxmemory.h"

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
}

void soft_body::update(float dt)
{
    m_camera.Update(dt);

    for (auto& body : dynamicbodies_tri) { body.update(dt); }
    for (auto& body : dynamicbodies_line) { body.update(dt); }

    static std::size_t const num_dynamicbodies = dynamicbodies_tri.size();
    for (std::size_t i = 0; i < num_dynamicbodies; ++i)
    {
        for (std::size_t j = i + 1; j < num_dynamicbodies; ++j)
        {
            dynamicbodies_tri[i].get().resolve_collision(dynamicbodies_tri[j].get(), dt);
        }
    }

    XMMATRIX world = XMMATRIX(g_XMIdentityR0, g_XMIdentityR1, g_XMIdentityR2, g_XMIdentityR3);
    XMMATRIX view = m_camera.GetViewMatrix();
    XMMATRIX proj = m_camera.GetProjectionMatrix(XM_PI / 3.0f, engine->get_config_properties().get_aspect_ratio());

    XMFLOAT3 cameraPos = m_camera.GetCurrentPosition();
    XMStoreFloat3(&(constbufferdata.CamPos), XMLoadFloat3(&cameraPos));
    XMStoreFloat4x4(&(constbufferdata.World), XMMatrixTranspose(world));
    XMStoreFloat4x4(&(constbufferdata.WorldView), XMMatrixTranspose(world * view));
    XMStoreFloat4x4(&(constbufferdata.WorldViewProj), XMMatrixTranspose(world * view * proj));

    cbuffer.set_data(&constbufferdata);
}

void soft_body::render(float dt)
{
    for (auto& body : dynamicbodies_tri) { body.render(dt, { m_wireframe_toggle, cbuffer.get_gpuaddress()}); }
    for (auto& body : dynamicbodies_line) { body.render(dt, { m_wireframe_toggle, cbuffer.get_gpuaddress() }); }
    for (auto& body : staticbodies_lines) { body.render(dt, { m_wireframe_toggle, cbuffer.get_gpuaddress() }); }
}

game_base::resourcelist soft_body::load_assets_and_geometry()
{
    auto device = engine->get_device();

    cbuffer.createresources<SceneConstantBuffer>();

    //static const std::uniform_real_distribution<float> color_dist(0.f, 1.f);
    //Vector3 const color = { color_dist(mt), color_dist(mt), color_dist(mt) };

    Vector3 const lposition = { -5.f, 0.f, 0.f };
    Vector3 const rposition = { 5.f, 0.f, 0.f };

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