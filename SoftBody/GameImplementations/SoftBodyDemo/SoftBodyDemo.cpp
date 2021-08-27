#include "softbodydemo.h"
#include "gameutils.h"
#include "sharedconstants.h"
#include "engine/interfaces/engineinterface.h"
#include "engine/DXSample.h"
#include "engine/engineutils.h"
#include "engine/geometry/geoutils.h"
#include "engine/graphics/body.h"
#include "engine/graphics/gfxmemory.h"

#include <set>
#include <ranges>
#include <algorithm>
#include <functional>

#define PROFILE_TIMING

#ifdef PROFILE_TIMING
#include <chrono>
using namespace std::chrono;
#endif 

#ifdef PROFILE_TIMING

#define TIMEDSCOPE(name) { steady_clock::time_point timed##name = high_resolution_clock::now();
#define TIMEDSCOPEEND(name) { \
auto t = duration_cast<microseconds>(high_resolution_clock::now() - timed##name);  \
char buf[512];                                                           \
sprintf_s(buf, "time for %s [%d]us\n", #name, static_cast<int>(t.count()));     \
OutputDebugStringA(buf);                                                 \
} \
}

#endif // PROFILE_TIMING
namespace game_creator
{
    template <>
    std::unique_ptr<game_base> create_instance<game_types::soft_body_demo>(game_engine const* engine)
    {
        return std::move(std::make_unique<soft_body>(engine));
    }
}

namespace gameparams
{
    constexpr float speed = 15.f;
    constexpr uint numballs = 75;
    constexpr float ballradius = 1.7f;
}

struct cell
{
    uint x = 0u;
    uint y = 0u;
    uint z = 0u;

    // degree is number of cells in a square grid in any dimension
    uint to1D(uint degree)
    {
        return (y * degree + z) * degree + x;
    }

    static cell to3D(uint v, uint degree)
    {
        cell res;
        res.x = v % degree;
        res.z = (v / degree) % degree;
        res.y = (v - res.x - (res.z * degree)) / (degree * degree);
        return res;
    }
};

std::vector<vec3> fillwithspheres(geometry::aabb const& box, uint count, float radius)
{
    std::set<uint> occupied;
    std::vector<vec3> spheres;

    auto const roomspan = (box.span() - geoutils::tolerance<vec3>);

    assert(roomspan.x > 0.f);
    assert(roomspan.y > 0.f);
    assert(roomspan.z > 0.f);

    uint const degree = static_cast<uint>(std::ceil(std::cbrt(count)));
    uint const numcells = degree * degree * degree;
    float const gridvol = numcells * 8 * (radius * radius * radius);

    // find the size of largest cube that can be fit into box
    float const cubelen = std::min({ roomspan.x, roomspan.y, roomspan.z });
    float const celld = cubelen / degree;
    float const cellr = celld / 2.f;;

    // check if this box can contain all voxels
    assert(cubelen * cubelen * cubelen > gridvol);

    vec3 const gridorigin = { box.center() - vec3(cubelen / 2.f) };
    for (auto i : std::ranges::iota_view{ 0u,  count })
    {
        static auto& re = engineutils::getrandomengine();
        static const std::uniform_int_distribution<uint> distvoxel(0u, numcells - 1);

        uint emptycell = 0;
        while (occupied.find(emptycell) != occupied.end())
            emptycell = distvoxel(re);

        occupied.insert(emptycell);

        cell const thecell = cell::to3D(emptycell, degree);
        spheres.emplace_back(vec3{ thecell.x * celld + cellr, thecell.y * celld + cellr, thecell.z * celld + cellr } + gridorigin);
    }

    return spheres;
}

using namespace DirectX;
using Microsoft::WRL::ComPtr;

soft_body::soft_body(game_engine const* engine)
	: game_base(engine)
{
    assert(engine != nullptr);

    m_camera.Init({ 0.f, 0.f, -15.f });
    m_camera.SetMoveSpeed(10.0f);
}

void soft_body::update(float dt)
{
    m_camera.Update(dt);

    auto const& roomaabb = staticbodies_boxes[0].getaabb();

    TIMEDSCOPE(selfcoll)
    for (uint i = 0; i < gameparams::numballs; ++i)
        for (uint j = i + 1; j < gameparams::numballs; ++j)
            balls[i].get().resolve_collision(balls[j].get(), dt);
    TIMEDSCOPEEND(selfcoll)

    TIMEDSCOPE(boxcoll)
    for (uint i = 0; i < gameparams::numballs; ++i)
        balls[i].get().resolve_collision_interior(roomaabb, dt);
    TIMEDSCOPEEND(boxcoll)

    TIMEDSCOPE(ballsupdate)
    for (auto& b : balls) b.update(dt);
    TIMEDSCOPEEND(ballsupdate)

    for (auto& b : dynamicbodies_line) b.update(dt);

    auto& viewinfo = gfx::getview();
    auto &constbufferdata = gfx::getglobals();

    viewinfo.view = m_camera.GetViewMatrix();
    viewinfo.proj = m_camera.GetProjectionMatrix(XM_PI / 3.0f, engine->get_config_properties().get_aspect_ratio());

    constbufferdata.campos = m_camera.GetCurrentPosition();
    constbufferdata.ambient = { 0.1f, 0.1f, 0.1f, 1.0f };
    constbufferdata.lights[0].direction = vec3{ 0.3f, -0.27f, 0.57735f };
    constbufferdata.lights[0].direction.Normalize();
    constbufferdata.lights[0].color = { 0.2f, 0.2f, 0.2f };

    // todo : shader bug?
    // point light at origin creates blurry lighting on room
    constbufferdata.lights[1].position = { 12.f, -12.f, -12.f};
    constbufferdata.lights[1].color = { 1.f, 1.f, 1.f };
    constbufferdata.lights[1].range = 20.f;

    constbufferdata.lights[2].position = {-10.f, 0.f, 0.f};
    constbufferdata.lights[2].color = { 0.6f, 0.8f, 0.6f };
    constbufferdata.lights[2].range = 20.f;

    cbuffer.set_data(&constbufferdata);
}

void soft_body::render(float dt)
{
    TIMEDSCOPE(render)
    for (auto& body : staticbodies_lines) { body.render(dt, { m_wireframe_toggle, cbuffer.get_gpuaddress() }); }
    for (auto& body : staticbodies_boxes) { body.render(dt, { m_wireframe_toggle, cbuffer.get_gpuaddress() }); }
    for (auto& body : dynamicbodies_line) { body.render(dt, { m_wireframe_toggle, cbuffer.get_gpuaddress() }); }
    for (auto& body : balls) { body.render(dt, { m_wireframe_toggle, cbuffer.get_gpuaddress() }); }
    TIMEDSCOPEEND(render)
}

game_base::resourcelist soft_body::load_assets_and_geometry()
{
    using geometry::cube;
    using gfx::bodyparams;
    using geometry::ffd_object;

    auto device = engine->get_device();
    cbuffer.createresources<gfx::sceneconstants>();

    staticbodies_boxes.emplace_back(cube{ {vec3{0.f, 0.f, 0.f}}, vec3{30.f} }, &cube::get_vertices_flipped, &cube::getinstancedata, bodyparams{ "instanced" });
    
    //dynamicbodies_line.emplace_back(*dynamicbodies_tri[0], &ffd_object::getbox_vertices, bodyparams{ "lines"});
    //dynamicbodies_line.emplace_back(*dynamicbodies_tri[1], &ffd_object::getbox_vertices, bodyparams{ "lines",});
    //staticbodies_lines.emplace_back(*dynamicbodies_tri[0], &ffd_object::get_control_point_visualization, &ffd_object::get_controlnet_instancedata, bodyparams{ "instancedlines" });
    //staticbodies_lines.emplace_back(*dynamicbodies_tri[1], &ffd_object::get_control_point_visualization, &ffd_object::get_controlnet_instancedata, bodyparams{ "instancedlines" });

    auto const& roomaabb = staticbodies_boxes[0].getaabb();
    auto const roomhalfspan = (roomaabb.max_pt - roomaabb.min_pt - (vec3::One * gameparams::ballradius) - vec3(0.5f)) / 2.f;

    static auto& re = engineutils::getrandomengine();
    static const std::uniform_real_distribution<float> distvelocity(-1.f, 1.f);

    static const auto basemat_ball = gfx::getmat("ball");
    for (auto const& center : fillwithspheres(roomaabb, gameparams::numballs, gameparams::ballradius))
    {
        auto velocity = vec3{ distvelocity(re), distvelocity(re), distvelocity(re) };
        velocity.Normalize();
        balls.emplace_back(ffd_object{ { center, gameparams::ballradius } }, bodyparams{ "default", gfx::generaterandom_matcolor(basemat_ball) });
        balls.back()->set_velocity(velocity * gameparams::speed);
    }

    //balls.emplace_back(ffd_object{ { {0.f, 0.f, 0.f}, gameparams::ballradius } }, bodyparams{ "default", gfx::generaterandom_matcolor(basemat_ball) });
    //balls.back()->set_velocity({15.f, 0.f, 0.f});

    //balls.emplace_back(ffd_object{ { {-10.f, 0.f, 0.f}, gameparams::ballradius } }, bodyparams{ "default", gfx::generaterandom_matcolor(basemat_ball) });
    //balls.back()->set_velocity({ -15.f, 0.f, 0.f });
    
    // todo : create an iterator to iterate over multiple ranges using as a common type
    game_base::resourcelist retvalues;
    for (auto& b : balls) { stdx::append(b.create_resources(), retvalues); };
    for (auto& b : dynamicbodies_line) { stdx::append(b.create_resources(), retvalues); };
    for (auto& b : staticbodies_lines) { stdx::append(b.create_resources(), retvalues); };
    for (auto& b : staticbodies_boxes) { stdx::append(b.create_resources(), retvalues); };

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