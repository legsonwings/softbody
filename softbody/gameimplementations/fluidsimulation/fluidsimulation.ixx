module;

#include "engine/stdx.h"
#include "engine/engineutils.h"
#include "engine/engine.h"
#include "engine/graphics/gfxcore.h"
#include "engine/graphics/gfxmemory.h"
#include "engine/graphics/body.h"
#include "engine/interfaces/bodyinterface.h"
#include "engine/geometry/geoutils.h"
#include "engine/geometry/geocore.h"
#include "engine/cursor.h"
#include "gamebase.h"
#include "gameutils.h"
#include "fluidcore.h"

export module fluidsimulation;

import shapes;

using namespace DirectX;
using Microsoft::WRL::ComPtr;

namespace fluid
{
static constexpr float maxd = 1.f;
using cubeidx = stdx::hypercubeidx<1>;

template <typename t, uint d, uint vd, uint l>
concept vecfieldrefvalue_c = std::is_same_v<std::decay_t<t>, vecfield<d, vd, l>>;

template<typename t, uint l>
requires vecfieldrefvalue_c<t, 1, 0, l>
struct scalarfieldro
{
    t sf;
    static constexpr vec2 origin = -vec2{ l / 2.f, l / 2.f };

    std::vector<geometry::vertex> gvertices() const
    {
        float const z = 0.f;
        static const vector3 normal = vector3::Forward;
        std::vector<geometry::vertex> r;
        //r.push_back(geometry::vertex{ {-0.5, 0.5, z}, normal });
        //r.push_back(geometry::vertex{ {0.5, 0.5, z}, normal });
        //r.push_back(geometry::vertex{ {-0.5, -0.5, z}, normal });
        //r.push_back(geometry::vertex{ {-0.5, -0.5, z}, normal });
        //r.push_back(geometry::vertex{ {0.5, 0.5, z}, normal });
        //r.push_back(geometry::vertex{ {0.5, -0.5, z}, normal });

        r.push_back(geometry::vertex{ {-200.f, 200.f, z}, normal });
        r.push_back(geometry::vertex{ {200.f, 200.f, z}, normal });
        r.push_back(geometry::vertex{ {-200.f, -200.f, z}, normal });
        r.push_back(geometry::vertex{ {-200.f, -200.f, z}, normal });
        r.push_back(geometry::vertex{ {200.f, 200.f, z}, normal });
        r.push_back(geometry::vertex{ {200.f, -200.f, z}, normal });

        return r;
    }

    std::vector<gfx::instance_data> instancedata() const
    {
        gfx::material mat = gfx::getmat("");
        std::vector<gfx::instance_data> r;
        mat.diffuse({ 0.5, 0.5f, 0.f, 1.f });
        //for (uint i(0); i < sf.size(); ++i)
        //{
        //    float const c = stdx::lerp(0.f, 1.f, sf[i] / maxd);
        //    auto const idx = cubeidx::from1d(l - 1, i).coords;
        //    auto const pos = vec2{ static_cast<float>(idx[0]), static_cast<float>(idx[1]) } + origin;
        //    mat.diffuse({c, 0.f, 0.f, 1.f});

        //    r.push_back(gfx::instance_data(matrix::CreateTranslation({ pos[0], pos[1], -10.f }), gfx::getview(), mat));
        //}

        r.push_back(gfx::instance_data(matrix::CreateTranslation({ 0.f, 0.f, -42.8f }), gfx::getview(), mat));

        return r;
    }
};

struct fluidtex
{
    std::vector<geometry::vertex> gvertices() const
    {
        return _quad.triangles();
    }

    void update(float dt)
    {

    }

    vector3 gcenter() const
    {
        return { 0.f, 0.f, -40.f };
    }

    std::vector<uint8_t> texdata()
    {
        static_assert(sizeof(float) == 4);

        if (_texdata.size() == 0)
        {
            _texdata.resize(10000 * 3 * 4);

            std::vector<float> dataf(10000 * 3, 1.f);
            memcpy(_texdata.data(), dataf.data(), 10000 * 3 * 4);
        }

        return _texdata;
    }

    std::vector<uint8_t> _texdata;

    // todo : move this z to origin. Doesn't matter as long as we are within view frustum
    geometry::rectangle _quad{ 100.f, 100.f,  gcenter() };
};

class fluidsimulation : public game_base
{
public:
    fluidsimulation(gamedata const& data) : game_base(data)
    {
        camera.Init({ 0.f, 0.f, -43.f });
        camera.SetMoveSpeed(10.0f);
    }

    cursor cursor;
private:
    static constexpr uint vd = 1;
    static constexpr uint l = 70;

    fluidbox<vd, l> fluid{ 0.2f};
    //std::vector<gfx::body_static<geometry::cube, gfx::topology::triangle>> boxes;
    std::vector<gfx::body_static<scalarfieldro<vecfield21<l> const&, l>, gfx::topology::triangle>> dye;
    std::vector<gfx::body_dynamic<fluidtex, gfx::topology::triangle>> textures;

    gfx::resourcelist load_assets_and_geometry() override
    {
        using geometry::cube;
        using gfx::bodyparams;
        cbuffer.createresources<gfx::sceneconstants>();

        //boxes.emplace_back(cube{ {vector3{0.f, 0.f, 0.f}}, vector3{40.f} }, &cube::vertices_flipped, &cube::instancedata, bodyparams{ "instanced" });
        //dye.emplace_back(scalarfieldro<vecfield21<l> const& ,l>{fluid.d}, bodyparams{ "instanced" });

        textures.emplace_back(fluidtex{}, bodyparams{ "texturess", "", {100, 100}}).texturedata(textures.back()->texdata());

        gfx::resourcelist r;
        for (auto b : stdx::makejoin<gfx::bodyinterface>(textures)) { stdx::append(b->create_resources(), r); };
        return r;
    }

    void update(float dt) override
    {
        game_base::update(dt);

        gfx::getview().proj = camera.GetOrthoProjectionMatrix();

        auto& constbufferdata = gfx::getglobals();

        constbufferdata.campos = camera.GetCurrentPosition();
        // todo : lights to be used as global dispatch param
        constbufferdata.ambient = { 0.1f, 0.1f, 0.1f, 1.0f };
        constbufferdata.lights[0].direction = vector3{ 0.3f, -0.27f, 0.57735f }.Normalized();
        constbufferdata.lights[0].color = { 0.2f, 0.2f, 0.2f };

        constbufferdata.lights[1].position = vector3::Zero;
        constbufferdata.lights[1].color = vector3::Zero;
        constbufferdata.lights[1].range = 40.f;

        constbufferdata.lights[2].position = vector3::Zero;
        constbufferdata.lights[2].color = vector3::Zero;
        constbufferdata.lights[2].range = 40.f;

        cbuffer.set_data(&constbufferdata);

        cursor.tick(dt);

        DirectX::SimpleMath::Ray ray(camera.GetCurrentPosition(), cursor.ray(camera.nearplane(), camera.farplane()));
        
        float dist;
        if (ray.Intersects(plane{ vector3::Zero, vector3::Backward }, dist))  // fluid box is at origin
        {
            vector3 const point = ray.position + ray.direction * dist;

            static constexpr float speed = 2.5f;
            static constexpr DirectX::SimpleMath::Vector2 origin = DirectX::SimpleMath::Vector2{ -(l / 2.f), -(l / 2.f) };

            // todo : implement the fluid plane as fulscreen canvas
            // todo : provide helpers to do type conversions
            // todo : implement hypercubeidx as child of std::array
            DirectX::SimpleMath::Vector2 const idxf = { std::roundf(point.x - origin.x), std::roundf(point.y - origin.y) };
            cubeidx const idx = { static_cast<uint>(idxf.x), static_cast<uint>(idxf.y)};

            uint const spread = 1;
            for (uint i(idx[0] - spread); i <= (idx[0] + spread) && (i > 0 && i < (l - 1)); ++i)
                for (uint j(idx[1] - spread); j <= (idx[1] + spread) && (j > 0 && j < (l - 1)); ++j)
                {
                    DirectX::SimpleMath::Vector2 const currentcellf = { static_cast<float>(i), static_cast<float>(j) };
                    auto const vel = (currentcellf - idxf).Normalized() * speed + cursor.vel() * 0.25f;
                    fluid.adddensity({ i,j }, 0.2f);
                    fluid.addvelocity({ i,j }, { vel.x, vel.y });
                }
        }

        auto& v = fluid.v;
        v = advect2d(v, v, dt);
        v = diffuse({}, v, dt, 0.4f);  // this new field has divergence

        // compute the pressure field, solving the linear equation : lap(p) = div(v)
        vecfield21<l> p = {};
        p = jacobi2d(p, divergence<l>(v), 4, 1.f, 4.f);

        // per helmholtz-hodge, decompose divergent field into one without it and pressure gradient
        v = v - gradient(p);  // this is the divergence free field
        sbounds(v, -1.f);

        fluid.d = advect2d(fluid.d, v, dt);
        fluid.d = diffuse({}, fluid.d, dt, 0.5f);
        sbounds(fluid.d, 1.f);
    }

    void render(float dt) override
    {
        //for (auto b : stdx::makejoin<gfx::bodyinterface>(dye))  b->render(dt, { false, cbuffer.get_gpuaddress() });
    }
};
}

namespace game_creator
{
    template <>
    std::unique_ptr<game_base> create_instance<game_types::fluidsimulation>(gamedata const& data) { return std::move(std::make_unique<fluid::fluidsimulation>(data)); }
}