#include "stdafx.h"
#include "softbodydemo.h"
#include "gameutils.h"
#include "sharedconstants.h"
#include "engine/interfaces/engineinterface.h"
#include "engine/DXSample.h"
#include "engine/engineutils.h"
#include "engine/graphics/body.h"
#include "engine/graphics/gfxmemory.h"

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

soft_body::soft_body(game_engine const* engine)
	: game_base(engine)
{
    assert(engine != nullptr);

    m_camera.Init({ 0.f, 0.f, -8.f });
    m_camera.SetMoveSpeed(10.0f);
}

void soft_body::update(float dt)
{
    m_camera.Update(dt);

    for (auto& body : balls) { body.update(dt); }
    for (auto& body : bubbles) { body.update(dt); }
    for (auto& body : dynamicbodies_line) { body.update(dt); }

    // todo : balls and bubbles don't collide just yet
    static std::size_t const numballs = balls.size();
    for (std::size_t i = 0; i < numballs; ++i)
    {
        for (std::size_t j = i + 1; j < numballs; ++j)
        {
            balls[i].get().resolve_collision(balls[j].get(), dt);
        }
    }

    static std::size_t const numbubbles = bubbles.size();
    for (std::size_t i = 0; i < numbubbles; ++i)
    {
        for (std::size_t j = i + 1; j < numbubbles; ++j)
        {
            bubbles[i].get().resolve_collision(bubbles[j].get(), dt);
        }
    }

    auto& viewinfo = gfx::getview();
    auto &constbufferdata = gfx::getglobals();

    viewinfo.view = m_camera.GetViewMatrix();
    viewinfo.proj = m_camera.GetProjectionMatrix(XM_PI / 3.0f, engine->get_config_properties().get_aspect_ratio());

    constbufferdata.campos = m_camera.GetCurrentPosition();
    constbufferdata.ambient = { 0.5f, 0.25f, 0.47f, 1.0f };
    constbufferdata.lights[0].direction = vec3{ 0.3f, -0.27f, 0.57735f };
    constbufferdata.lights[0].direction.Normalize();
    constbufferdata.lights[0].color = { 0.4f, 0.5f, 0.5f };
    constbufferdata.lights[1].position = { 7.f, 0.f, -2.f};
    constbufferdata.lights[1].color = { 0.8f, 0.3f, 0.3f };
    constbufferdata.lights[1].range = 25.f;

    cbuffer.set_data(&constbufferdata);
}

void soft_body::render(float dt)
{
    // sort transparent objects by depth
    std::sort(bubbles.begin(), bubbles.end(), [worldtoview = gfx::getview().view](const auto &l, const auto& r)
        {
            return vec3::Transform(l->getcenter(), worldtoview).z > vec3::Transform(r->getcenter(), worldtoview).z;
        });

    for (auto& body : staticbodies_lines) { body.render(dt, { m_wireframe_toggle, cbuffer.get_gpuaddress() }); }
    for (auto& body : staticbodies_boxes) { body.render(dt, { m_wireframe_toggle, cbuffer.get_gpuaddress() }); }
    for (auto& body : dynamicbodies_line) { body.render(dt, { m_wireframe_toggle, cbuffer.get_gpuaddress() }); }
    for (auto& body : balls) { body.render(dt, { m_wireframe_toggle, cbuffer.get_gpuaddress() }); }
    for (auto& body : bubbles) { body.render(dt, { m_wireframe_toggle, cbuffer.get_gpuaddress() }); }
}

game_base::resourcelist soft_body::load_assets_and_geometry()
{
    auto device = engine->get_device();

    cbuffer.createresources<gfx::sceneconstants>();

    vec3 const lpos = { -5.f, 0.f, 0.f };
    vec3 const rpos = { 5.f, 0.f, 0.f };
    vec3 const bpos = { 0.f, 0.f, 0.f };

    using geometry::cube;
    using gfx::bodyparams;
    using geometry::ffd_object;
    //balls.emplace_back(ffd_object{ { lposition, 1.f } }, bodyparams{ "default", "ball" });
    //balls.emplace_back(ffd_object{ { rposition, 1.f } }, bodyparams{ "default", "ball" });
    //balls[0].get().set_velocity({ 8.f, 0.f, 0.f });
    //balls[1].get().set_velocity({ -8.f, 0.f, 0.f });

    static const auto basemat = gfx::getmat("transparentball_twosided");
    bubbles.emplace_back(ffd_object{ { lpos, 1.f } }, bodyparams{ "transparent_twosided", gfx::generaterandom_matcolor(basemat) });
    bubbles.emplace_back(ffd_object{ { rpos, 1.f } }, bodyparams{ "transparent_twosided",  gfx::generaterandom_matcolor(basemat) });
    bubbles.emplace_back(ffd_object{ { bpos, 1.f } }, bodyparams{ "transparent_twosided", gfx::generaterandom_matcolor(basemat) });
    bubbles[0].get().set_velocity({ 10.f, 0.f, 0.f });
    bubbles[1].get().set_velocity({ -9.f, 0.f, 0.f });
    bubbles[2].get().set_velocity({ 0.f, 0.f, 0.f });

    //dynamicbodies_line.emplace_back(*dynamicbodies_tri[0], &ffd_object::getbox_vertices, bodyparams{ "lines"});
    //dynamicbodies_line.emplace_back(*dynamicbodies_tri[1], &ffd_object::getbox_vertices, bodyparams{ "lines",});
    //staticbodies_lines.emplace_back(*dynamicbodies_tri[0], &ffd_object::get_control_point_visualization, &ffd_object::get_controlnet_instancedata, bodyparams{ "instancedlines" });
    //staticbodies_lines.emplace_back(*dynamicbodies_tri[1], &ffd_object::get_control_point_visualization, &ffd_object::get_controlnet_instancedata, bodyparams{ "instancedlines" });
    staticbodies_boxes.emplace_back(cube{ {vec3{0.f, 0.f, 0.f}}, vec3{20.f} }, &cube::get_vertices_invertednormals, &cube::getinstancedata, bodyparams{ "instanced" });

    game_base::resourcelist retvalues;
    for (auto& b : balls) { xstd::append(b.create_resources(), retvalues); };
    for (auto& b : bubbles) { xstd::append(b.create_resources(), retvalues); };
    for (auto& b : dynamicbodies_line) { xstd::append(b.create_resources(), retvalues); };
    for (auto& b : staticbodies_lines) { xstd::append(b.create_resources(), retvalues); };
    for (auto& b : staticbodies_boxes) { xstd::append(b.create_resources(), retvalues); };

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