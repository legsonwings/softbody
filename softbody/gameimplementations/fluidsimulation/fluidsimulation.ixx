module;

#include "stdx/stdx.h"
#include "stdx/vec.h"

#include "gamebase.h"
#include "gameutils.h"
#include "fluidcore.h"

#include "engine/cursor.h"
#include "engine/engine.h"
#include "engine/engineutils.h"
#include "engine/graphics/body.h"
#include "engine/geometry/geocore.h"
#include "engine/graphics/gfxcore.h"
#include "engine/graphics/globalresources.h"
#include "engine/interfaces/bodyinterface.h"

#include <vector>
#include <cstdint>

export module fluidsimulation;

import shapes;

using namespace DirectX;
using Microsoft::WRL::ComPtr;
using stdx::vec2;

namespace fluid
{
static constexpr float maxd = 0.2f;
using cubeidx = stdx::grididx<1>;

struct fluidtex
{
    void update(float dt) {}
    std::vector<geometry::vertex> vertices() const { return _quad.triangles(); }
    vector3 center() const { return { vector3::Zero }; }

    fluidtex(stdx::vecui2 dims, uint texelsize = sizeof(uint32_t)) : _dims(dims), _texelsize(texelsize), _quad{ dims.castas<float>(), center() } {}

    stdx::vecui2 texpos(stdx::vecui2 fluidpos, stdx::vecui2 fluiddims)
    {
        float const scale0 = static_cast<float>(fluiddims[0]) / static_cast<float>(_dims[0]);
        float const scale1 = static_cast<float>(fluiddims[1]) / static_cast<float>(_dims[1]);

        return stdx::vec2{ fluidpos[0] * scale0, fluidpos[1] * scale1 }.castas<uint>();
    }

    stdx::vecui2 fluidpos(stdx::vecui2 texpos, stdx::vecui2 fluiddims)
    {
        float const scale0 = static_cast<float>(_dims[0]) / static_cast<float>(fluiddims[0]);
        float const scale1 = static_cast<float>(_dims[1]) / static_cast<float>(fluiddims[1]);

        return stdx::vec2{ texpos[0] * scale0, texpos[1] * scale1 }.castas<uint>();
    }
    
    std::vector<uint8_t> const& texturedata() const { return _texdata; }

    stdx::vecui2 _dims;
    uint _texelsize;
    geometry::rectangle _quad;

    std::vector<uint8_t> _texdata;
};

class fluidsimulation : public game_base
{
public:
    fluidsimulation(gamedata const& data) : game_base(data)
    {
        camera.Init({ 0.f, 0.f, -10.f });
        camera.lock(true);
    }

    cursor cursor;
private:
    static constexpr uint vd = 1;
    static constexpr uint l = 400;
    
    using fluid_t = fluidbox<vd, l>;
    static constexpr uint numcolors = decltype(fluid_t::d)::value_type::nd + 1;

    static constexpr stdx::vecui2 texdims{720, 720};

    uint _currentcolor = 0;
    fluidbox<vd, l> _fluid;
    gfx::body_dynamic<fluidtex, gfx::topology::triangle> _texture{ {texdims},  gfx::bodyparams{ "texturess", "", texdims} };

    void on_key_up(unsigned key) override
    {
        game_base::on_key_up(key);

        if(key == 'C') _currentcolor = (++_currentcolor) % numcolors;
    };

    gfx::resourcelist load_assets_and_geometry() override
    {
        _texture->_texdata.reserve(_texture->_dims[0] * _texture->_dims[1] * _texture->_texelsize);

        updatetexture();
        return _texture.create_resources();
    }

    void update(float dt) override
    {
        game_base::update(dt);

        gfx::globalresources::get().view().proj = camera.GetOrthoProjectionMatrix();
        gfx::globalresources::get().cbuffer().data().campos = camera.GetCurrentPosition();
        gfx::globalresources::get().cbuffer().updateresource();

        cursor.tick(dt);

        auto const pos = cursor.posicentered();
        if (std::abs(pos[0]) <= (texdims[0] / 2) && std::abs(pos[1]) <= (texdims[1] / 2))
        {
            static constexpr float speed = 2.5f;
            static constexpr auto origin = stdx::veci2{ -static_cast<int>(texdims[0] / 2), static_cast<int>(texdims[1] / 2)};
            cubeidx const idx = _texture->texpos(stdx::veci2{ pos[0] - origin[0], origin[1] - pos[1]}.castas<uint>(), {l, l});
            vector2 const idxf = { static_cast<float>(idx[0]), static_cast<float>(idx[1]) };

            uint const spread = 5;
            for (uint i(idx[0] - spread); i <= (idx[0] + spread) && (i > 0 && i < (l - 1)); ++i)
                for (uint j(idx[1] - spread); j <= (idx[1] + spread) && (j > 0 && j < (l - 1)); ++j)
                {
                    vector2 const currentcellf = { static_cast<float>(i), static_cast<float>(j) };
                    auto const vel = (currentcellf - idxf).Normalized() * speed + cursor.vel() * 0.1f;
                    _fluid.adddensity({ i, j }, _currentcolor, maxd);
                    _fluid.addvelocity({ i, j }, { vel.x, vel.y });
                }
        }

        auto& v = _fluid.v;
        v = advect2d(v, v, dt);
        v = diffuse({}, v, dt, 0.5f);  // this new field has divergence

        // compute the pressure field, solving the linear equation : lap(p) = div(v)
        vecfield21<l> p = {};
        p = jacobi2d(p, divergence<l>(v), 4, 1.f, 4.f);

        // per helmholtz-hodge decomposition, decompose divergent field into one without it and pressure gradient
        v = v - gradient(p);  // this is the divergence free field
        bound(v, -1.f);

        // diffuse dyes
        _fluid.d = advect2d(_fluid.d, v, dt);
        _fluid.d = diffuse({}, _fluid.d, dt, 0.5f);
        
        bound(_fluid.d, 1.f);

        updatetexture();
    }

    uint32_t packcolor(stdx::vec3 color)
    {
        static constexpr uint32_t a = 255;
        auto const r = static_cast<uint32_t>(color[0] * 255);
        auto const g = static_cast<uint32_t>(color[1] * 255);
        auto const b = static_cast<uint32_t>(color[2] * 255);
        return a << 24 | b << 16 | g << 8 | r;
    }


    void updatetexture()
    {
        _texture->_texdata.clear();
        float const scale0 = static_cast<float>(l) / static_cast<float>(_texture->_dims[0]);
        float const scale1 = static_cast<float>(l) / static_cast<float>(_texture->_dims[1]);

        for (uint j(0); j < _texture->_dims[1]; ++j)
            for(uint i(0); i < _texture->_dims[0]; ++i)
            {
                auto const fluidcell = cubeidx::to1d<l - 1>(cubeidx{ i * scale0, j * scale1 });
                uint32_t const color = packcolor({ stdx::clamp(_fluid.d[fluidcell] / maxd , 0.f, 1.f)});
                uint8_t const *den = reinterpret_cast<uint8_t const*>(&color);
                _texture->_texdata.insert(_texture->_texdata.end(), den, den + sizeof(color));
            }
    }

    void render(float dt) override
    {
       _texture.render(dt, { false });
    }
};
}

namespace game_creator
{
    template <> std::unique_ptr<game_base> create_instance<game_types::fluidsimulation>(gamedata const& data) { return std::move(std::make_unique<fluid::fluidsimulation>(data)); }
}