#include "softbodydemo.h"
#include "stdx/stdx.h"
#include "engine/engineutils.h"
#include "engine/graphics/body.h"
#include "engine/graphics/globalresources.h"

#include "gameutils.h"

#include <utility>
#include <ranges>
#include <algorithm>
#include <functional>
#include <unordered_set>

namespace game_creator
{
    template<> std::unique_ptr<game_base> create_instance<game_types::softbodydemo>(gamedata const& data) { return std::move(std::make_unique<soft_body>(data)); }
}

namespace gameparams
{
    constexpr float speed = 10.f;
    constexpr uint numballs = 80;
    constexpr float ballradius = 2.5f;
}

geometry::ffddata createffddata(geometry::shapeffd_c auto shape)
{ 
    auto const center = shape.gcenter();
    shape.scenter(vector3::Zero);
    shape.generate_triangles();
    return { center, shape.triangles() };
}

std::vector<vector3> fillwithspheres(geometry::aabb const& box, uint count, float radius)
{
    std::unordered_set<uint> occupied;
    std::vector<vector3> spheres;

    auto const roomspan = (box.span() - stdx::tolerance<vector3>);

    assert(roomspan.x > 0.f);
    assert(roomspan.y > 0.f);
    assert(roomspan.z > 0.f);

    uint const degree = static_cast<uint>(std::ceil(std::cbrt(count))) - 1;
    uint const numcells = (degree + 1) * (degree + 1) * (degree + 1);
    float const gridvol = numcells * 8 * (radius * radius * radius);

    // find the size of largest cube that can be fit into box
    float const cubelen = std::min({ roomspan.x, roomspan.y, roomspan.z });
    float const celld = cubelen / (degree + 1);
    float const cellr = celld / 2.f;;

    // check if this box can contain all cells
    assert(cubelen * cubelen * cubelen > gridvol);

    vector3 const gridorigin = { box.center() - vector3(cubelen / 2.f) };
    for (auto i : std::ranges::iota_view{ 0u,  count })
    {
        static auto& re = engineutils::getrandomengine();
        static std::uniform_int_distribution<uint> distvoxel(0u, numcells - 1);

        uint emptycell = 0;
        while (occupied.find(emptycell) != occupied.end()) emptycell = distvoxel(re);

        occupied.insert(emptycell);

        auto const thecell = stdx::grididx<2>::from1d(degree, emptycell);
        spheres.emplace_back((vector3(thecell[0] * celld, thecell[2] * celld, thecell[1] * celld) + vector3(cellr)) + gridorigin);
    }

    return spheres;
}

using namespace DirectX;
using Microsoft::WRL::ComPtr;

soft_body::soft_body(gamedata const& data) : game_base(data)
{
    camera.Init({ 0.f, 0.f, -43.f });
    camera.SetMoveSpeed(10.0f);
}

void soft_body::update(float dt)
{
    game_base::update(dt);

    auto const& roomaabb = boxes[0]->bbox();

    for (uint i = 0; i < gameparams::numballs; ++i)
        for (uint j = i + 1; j < gameparams::numballs; ++j)
            balls[i].get().resolve_collision(balls[j].get(), dt);

    for (uint i = 0; i < gameparams::numballs; ++i)
        balls[i].get().resolve_collision_interior(roomaabb, dt);

    for (auto b : stdx::makejoin<gfx::bodyinterface>(balls, reflines)) b->update(dt);

    gfx::globalresources::get().view().proj = camera.GetProjectionMatrix(XM_PI / 3.0f);
    gfx::globalresources::get().cbuffer().data().campos = camera.GetCurrentPosition();
    gfx::globalresources::get().cbuffer().updateresource();
}

void soft_body::render(float dt)
{
    for (auto b : stdx::makejoin<gfx::bodyinterface>(boxes, balls)) b->render(dt, { wireframe_toggle });
    if (debugviz_toggle) for (auto b : stdx::makejoin<gfx::bodyinterface>(reflines, refstaticlines)) b->render(dt, { wireframe_toggle });
}

gfx::resourcelist soft_body::load_assets_and_geometry()
{
    using geometry::cube;
    using geometry::sphere;
    using gfx::bodyparams;
    using gfx::material;
    using geometry::ffd_object;

    auto& globalres = gfx::globalresources::get();
    auto const ballmat = material().roughness(0.95f).diffuse(gfx::color::white).fresnelr(vector3{ 0.0f });
    auto const transparent_ballmat = material().roughness(0.f).diffuse({ 0.7f, 0.9f, 0.6f, 0.1f }).fresnelr(vector3{ 1.f });

    globalres.addmat("ball", ballmat);
    globalres.addmat("transparentball", transparent_ballmat);
    globalres.addmat("transparentball_twosided", transparent_ballmat, true);
    globalres.addmat("room", material().roughness(0.99f).diffuse({ 0.5f, 0.3f, 0.2f, 1.f }).fresnelr(vector3{ 0.f }));

    auto& globals = gfx::globalresources::get().cbuffer().data();

    // initialize lights
    globals.numdirlights = 1;
    globals.numpointlights = 2;

    globals.ambient = { 0.1f, 0.1f, 0.1f, 1.0f };
    globals.lights[0].direction = vector3{ 0.3f, -0.27f, 0.57735f }.Normalized();
    globals.lights[0].color = { 0.2f, 0.2f, 0.2f };

    globals.lights[1].position = { -15.f, 15.f, -15.f };
    globals.lights[1].color = { 1.f, 1.f, 1.f };
    globals.lights[1].range = 40.f;

    globals.lights[2].position = { 15.f, 15.f, -15.f };
    globals.lights[2].color = { 1.f, 1.f, 1.f };
    globals.lights[2].range = 40.f;

    gfx::globalresources::get().cbuffer().updateresource();

    boxes.emplace_back(cube{ {vector3{0.f, 0.f, 0.f}}, vector3{40.f} }, &cube::vertices_flipped, &cube::instancedata, bodyparams{ "instanced" });
    
    auto const& roomaabb = boxes[0]->bbox();
    static auto& re = engineutils::getrandomengine();
    static std::uniform_real_distribution<float> distvelocity(-1.f, 1.f);

    static const auto basemat_ball = gfx::globalresources::get().mat("ball");
    for (auto const& center : fillwithspheres(roomaabb, gameparams::numballs, gameparams::ballradius))
    {
        auto const velocity = vector3{ distvelocity(re), distvelocity(re), distvelocity(re) }.Normalized() * gameparams::speed;
        balls.emplace_back(ffd_object(createffddata(sphere{ center, gameparams::ballradius })), bodyparams{ "wireframe", gfx::generaterandom_matcolor(basemat_ball) });
        balls.back()->svelocity(velocity);
    }

    // ensure balls container will no longer be modified, since visualizations take references to objects in container
    for (auto const& b : balls)
    {
        reflines.emplace_back(*b, &ffd_object::boxvertices, bodyparams{ "lines" });
        refstaticlines.emplace_back(*b, &ffd_object::controlpoint_visualization, &ffd_object::controlnet_instancedata, bodyparams{ "instancedlines" });
    }

    gfx::resourcelist resources;
    for (auto b : stdx::makejoin<gfx::bodyinterface>(balls, boxes, reflines, refstaticlines)) { stdx::append(b->create_resources(), resources); };
    return resources;
}

void soft_body::on_key_down(unsigned key)
{
    game_base::on_key_down(key);

    if (key == 'T') wireframe_toggle = !wireframe_toggle;
    if (key == 'V') debugviz_toggle = !debugviz_toggle;
}