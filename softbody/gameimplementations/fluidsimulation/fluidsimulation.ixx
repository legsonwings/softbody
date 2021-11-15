module;

#include "engine/stdx.h"
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
#include "engine/win32application.h"

export module fluidsimulation;

import shapes;

using namespace DirectX;
using Microsoft::WRL::ComPtr;

namespace fluid
{
static constexpr float maxd = 1.f;
using cubeidx = stdx::hypercubeidx<1>;

template<uint l>
struct scalarfieldro
{
    vecfield21<l> sf;
    static constexpr fluid::vec2 origin = -fluid::vec2{ l / 2.f, l / 2.f };

    std::vector<geometry::vertex> gvertices() const
    {
        float const z = 0.f;
        static const vec3 normal = vec3::Forward;
        std::vector<geometry::vertex> r;
        r.push_back(geometry::vertex{ {-0.5, 0.5, z}, normal });
        r.push_back(geometry::vertex{ {0.5, 0.5, z}, normal });
        r.push_back(geometry::vertex{ {-0.5, -0.5, z}, normal });
        r.push_back(geometry::vertex{ {-0.5, -0.5, z}, normal });
        r.push_back(geometry::vertex{ {0.5, 0.5, z}, normal });
        r.push_back(geometry::vertex{ {0.5, -0.5, z}, normal });

        return r;
    }

    std::vector<gfx::instance_data> instancedata() const
    {
        gfx::material mat = gfx::getmat("room");
        std::vector<gfx::instance_data> r;
        for (uint i(0); i < sf.size(); ++i)
        {
            float const c = stdx::lerp(0.f, 1.f, sf[i] / maxd);
            auto const idx = cubeidx::from1d(l - 1, i).coords;
            auto const pos = fluid::vec2{ static_cast<float>(idx[0]), static_cast<float>(idx[1]) } + origin;
            mat.diffusealbedo({c, 0.f, 0.f, 1.f});

            r.push_back(gfx::instance_data(matrix::CreateTranslation({ pos[0], pos[1], 0.f }), gfx::getview(), mat));
        }

        return r;
    }
};

// todo : remove game_engine from game
// its only used by gfx

class fluidsimulation : public game_base
{
public:
    fluidsimulation(game_engine const* engine) : game_base(engine)
    {
        assert(engine != nullptr);

        m_camera.Init({ 0.f, 0.f, -43.f });
        m_camera.SetMoveSpeed(10.0f);
    }

    cursor cursor;
private:
    static constexpr uint vd = 1;
    static constexpr uint l = 70;

    fluidbox<vd, l> fluid{ 0.2f, 0.5f };
    std::vector<gfx::body_static<geometry::cube, gfx::topology::triangle>> boxes;
    std::vector<gfx::body_static<scalarfieldro<l>, gfx::topology::triangle>> dye;

    resourcelist load_assets_and_geometry() override
    {
        using geometry::cube;
        using gfx::bodyparams;
        cbuffer.createresources<gfx::sceneconstants>();

        boxes.emplace_back(cube{ {vec3{0.f, 0.f, 0.f}}, vec3{40.f} }, &cube::vertices_flipped, &cube::instancedata, bodyparams{ "instanced" });
        dye.emplace_back(scalarfieldro<l>{}, bodyparams{ "instanced" });

        //for (uint i((l / 2) - 3); i < (l / 2) + 3; ++i)
        //    for (uint j((l / 2) - 3); j < (l / 2) + 3; ++j)
        //    {
        //        fluid.adddensity({ i,j }, maxd);
        //        fluid.addvelocity({ i,j }, { 0.f, 4.f });
        //    }

        resourcelist r;
        for (auto b : stdx::makejoin<gfx::bodyinterface>(dye, boxes)) { stdx::append(b->create_resources(), r); };
        return r;
    }

    void update(float dt) override
    {
        m_camera.Update(dt);

        auto& viewinfo = gfx::getview();
        auto& constbufferdata = gfx::getglobals();

        viewinfo.view = m_camera.GetViewMatrix();
        viewinfo.proj = m_camera.GetProjectionMatrix(XM_PI / 3.0f, engine->get_config_properties().get_aspect_ratio());

        constbufferdata.campos = m_camera.GetCurrentPosition();
        constbufferdata.ambient = { 0.1f, 0.1f, 0.1f, 1.0f };
        constbufferdata.lights[0].direction = vec3{ 0.3f, -0.27f, 0.57735f }.Normalized();
        constbufferdata.lights[0].color = { 0.2f, 0.2f, 0.2f };

        constbufferdata.lights[1].position = { -15.f, 15.f, -15.f };
        constbufferdata.lights[1].color = { 1.f, 1.f, 1.f };
        constbufferdata.lights[1].range = 40.f;

        constbufferdata.lights[2].position = { 15.f, 15.f, -15.f };
        constbufferdata.lights[2].color = { 1.f, 1.f, 1.f };
        constbufferdata.lights[2].range = 40.f;

        cbuffer.set_data(&constbufferdata);

        cursor.tick(dt);

        DirectX::SimpleMath::Ray ray(m_camera.GetCurrentPosition(), cursor.ray());
        
        float dist;
        // fluid box is at origin
        if (ray.Intersects(plane{ vec3::Zero, vec3::Backward }, dist))
        {
            vec3 const point = ray.position + ray.direction * dist;

            static constexpr float speed = 2.f;
            static constexpr DirectX::SimpleMath::Vector2 origin = DirectX::SimpleMath::Vector2{ -(l / 2.f), -(l / 2.f) };

            // todo : account for window scaling when computing cursor pos
            // todo : implement the fluid plane as fulscreen canvas
            // todo : provide helpers to do type conversions
            // todo : implement hypercubeidx as child of std::array
            DirectX::SimpleMath::Vector2 const idxf = { std::roundf(point.x - origin.x), std::roundf(point.y - origin.y) };
            cubeidx const idx = { static_cast<uint>(idxf.x), static_cast<uint>(idxf.y)};

            float spread = 1.f;
            for (uint i(idx[0] - spread); i <= (idx[0] + spread) && (i > 0 && i < (l - 1)); ++i)
                for (uint j(idx[1] - spread); j <= (idx[1] + spread) && (i > 0 && i < (l - 1)); ++j)
                {
                    DirectX::SimpleMath::Vector2 const currentcellf = { static_cast<float>(i), static_cast<float>(j) };
                    auto const vel = (currentcellf - idxf).Normalized() * speed + cursor.vel() * 0.5f;
                    fluid.adddensity({ i,j }, 0.25);
                    fluid.addvelocity({ i,j }, { vel.x, vel.y });
                }
        }


        // todo : these probably need optimizations
        auto& v = fluid.v;
        v = advect2d(v, v, dt);
        sbounds(v, -1.f);
        
        v = diffuse({}, v, dt, 0.4f);
        sbounds(v, -1.f);

        // move this to a project function
        // the new field has divergence
        auto div = divergence<l>(v);
        sbounds(div, 1.f);

        // compute the pressure field
        vecfield21<l> p = {};
        p = jacobi2d(p, div, 4, 1.f, 4.f);
        sbounds(p, 1.f);

        // this is the divergence free field
        v = v - gradient(p);
        sbounds(v, -1.f);

        auto& d = fluid.d;
        d = advect2d(d, v, dt);
        d = diffuse({}, d, dt, 0.5f);
        sbounds(d, 1.f);

        // todo : this doesn't need to be copied
        dye[0]->sf = d;
    }

    void render(float dt) override
    {
        for (auto b : stdx::makejoin<gfx::bodyinterface>(dye))  b->render(dt, { false, cbuffer.get_gpuaddress() });
    }
};
}

namespace game_creator
{
    template <>
    std::unique_ptr<game_base> create_instance<game_types::fluidsimulation>(game_engine const* engine) { return std::move(std::make_unique<fluid::fluidsimulation>(engine)); }
}