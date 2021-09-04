#include "softbodydemo.h"
#include "gameutils.h"
#include "sharedconstants.h"
#include "engine/interfaces/engineinterface.h"
#include "engine/DXSample.h"
#include "engine/engineutils.h"
#include "engine/geometry/geoutils.h"
#include "engine/graphics/body.h"
#include "engine/graphics/gfxmemory.h"
#include "engine/geometry/beziershapes.h"

#include <set>
#include <ranges>
#include <algorithm>
#include <functional>

//#define PROFILE_TIMING

#ifdef PROFILE_TIMING
#include <chrono>
using namespace std::chrono;
#endif 

#ifdef PROFILE_TIMING

#define TIMEDSCOPE(name) { steady_clock::time_point timed##name = high_resolution_clock::now();
#define TIMEDSCOPEEND(name) {                                                      \
auto t = duration_cast<microseconds>(high_resolution_clock::now() - timed##name);  \
char buf[512];                                                                     \
sprintf_s(buf, "time for %s [%d]us\n", #name, static_cast<int>(t.count()));        \
OutputDebugStringA(buf);                                                           \
}                                                                                  \
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
    constexpr float speed = 10.f;
    constexpr uint numballs = 50;
    constexpr float ballradius = 1.8f;
}

std::vector<vec3> fillwithspheres(geometry::aabb const& box, uint count, float radius)
{
    std::set<uint> occupied;
    std::vector<vec3> spheres;

    auto const roomspan = (box.span() - stdx::tolerance<vec3>);

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

        using beziermaths::hypercubeidx;
        hypercubeidx<2> const thecell = hypercubeidx<2>::from1d<2>(emptycell);
        spheres.emplace_back(vec3{ thecell[0] * celld + cellr, thecell[1] * celld + cellr, thecell[2] * celld + cellr } + gridorigin);
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

    for (uint i = 0; i < gameparams::numballs; ++i)
        for (uint j = i + 1; j < gameparams::numballs; ++j)
            balls[i].get().resolve_collision(balls[j].get(), dt);

    for (uint i = 0; i < gameparams::numballs; ++i)
        balls[i].get().resolve_collision_interior(roomaabb, dt);

    for (auto& b : balls) b.update(dt);
    for (auto& b : dynamicbodies_line) b.update(dt);

    auto& viewinfo = gfx::getview();
    auto &constbufferdata = gfx::getglobals();

    viewinfo.view = m_camera.GetViewMatrix();
    viewinfo.proj = m_camera.GetProjectionMatrix(XM_PI / 3.0f, engine->get_config_properties().get_aspect_ratio());

    constbufferdata.campos = m_camera.GetCurrentPosition();
    constbufferdata.ambient = { 0.1f, 0.1f, 0.1f, 1.0f };
    constbufferdata.lights[0].direction = vec3{ 0.3f, -0.27f, 0.57735f }.Normalized();
    constbufferdata.lights[0].color = { 0.2f, 0.2f, 0.2f };

    // bug? : point light at origin creates blurry lighting on room
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
    for (auto& b : staticbodies_boxes) { b.render(dt, { wireframe_toggle, cbuffer.get_gpuaddress() }); }
    for (auto& b : balls) { b.render(dt, { wireframe_toggle, cbuffer.get_gpuaddress() }); }
    for (auto& b : bezier) { b.render(dt, { wireframe_toggle, cbuffer.get_gpuaddress() }); };

    if (debugviz_toggle)
    {
        for (auto& b : dynamicbodies_line) { b.render(dt, { wireframe_toggle, cbuffer.get_gpuaddress() }); }
        for (auto& b : staticbodies_lines) { b.render(dt, { wireframe_toggle, cbuffer.get_gpuaddress() }); }
    }
}

game_base::resourcelist soft_body::load_assets_and_geometry()
{
    using geometry::cube;
    using geometry::sphere;
    using gfx::bodyparams;
    using geometry::ffd_object;

    auto device = engine->get_device();
    cbuffer.createresources<gfx::sceneconstants>();

    staticbodies_boxes.emplace_back(cube{ {vec3{0.f, 0.f, 0.f}}, vec3{30.f} }, &cube::vertices_flipped, &cube::instancedata, bodyparams{ "instanced" });
    
    auto const& roomaabb = staticbodies_boxes[0].getaabb();
    auto const roomhalfspan = (roomaabb.max_pt - roomaabb.min_pt - (vec3::One * gameparams::ballradius) - vec3(0.5f)) / 2.f;

    static auto& re = engineutils::getrandomengine();
    static const std::uniform_real_distribution<float> distvelocity(-1.f, 1.f);

    static const auto basemat_ball = gfx::getmat("ball");
    for (auto const& center : fillwithspheres(roomaabb, gameparams::numballs, gameparams::ballradius))
    {
        auto const velocity = vec3{ distvelocity(re), distvelocity(re), distvelocity(re) }.Normalized() * gameparams::speed;
        balls.emplace_back(ffd_object{ sphere{ center, gameparams::ballradius } }, bodyparams{ "default", gfx::generaterandom_matcolor(basemat_ball) });
        balls.back()->svelocity(velocity);
    }

    // ensure balls container will no longer be modified, since it visualizations take references to objects in container
    for (auto const& b : balls)
    {
        dynamicbodies_line.emplace_back(*b, &ffd_object::boxvertices, bodyparams{ "lines" });
        staticbodies_lines.emplace_back(*b, &ffd_object::controlpoint_visualization, &ffd_object::controlnet_instancedata, bodyparams{ "instancedlines" });
    }

    using beziermaths::controlpoint;
    geometry::qbeziercurve qcurve;
    qcurve.curve.controlnet = std::array<controlpoint, 3>{controlpoint{ -1.f, 0.f, 0.f }, controlpoint{ 0.f, 2.f, 0.f }, controlpoint{ 1.f, 0.f, 0.f }};
    //bezier.emplace_back(qcurve, bodyparams{ "instancedlines" });

    game_base::resourcelist resources;
    for (auto& b : balls) { stdx::append(b.create_resources(), resources); };
    for (auto& b : dynamicbodies_line) { stdx::append(b.create_resources(), resources); };
    for (auto& b : staticbodies_lines) { stdx::append(b.create_resources(), resources); };
    for (auto& b : staticbodies_boxes) { stdx::append(b.create_resources(), resources); };
    for (auto& b : bezier) { stdx::append(b.create_resources(), resources); };

    return resources;
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
        wireframe_toggle = !wireframe_toggle;
    }

    if (key == 'V')
    {
        debugviz_toggle = !debugviz_toggle;
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