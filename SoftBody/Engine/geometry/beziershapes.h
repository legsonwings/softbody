#pragma once

#include "engine/stdx.h"
#include "beziermaths.h"
#include "engine/graphics/gfxcore.h"
#include "engine/graphics/gfxutils.h"

#include <vector>

namespace geometry
{
    struct qbeziercurve
    {
        std::vector<vec3> gvertices() const 
        { 
            std::vector<vec3> res;
            for (auto const& v : beziermaths::tessellate(curve))
                res.push_back(v.first);

            return res;
        }

        std::vector<gfx::instance_data> instancedata() const { return { gfx::instance_data(matrix::CreateTranslation(vec3::Zero), gfx::getview(), gfx::getmat("")) }; }

        beziermaths::beziercurve<2u> curve;
    };

    struct qbeziervolume
    {
        std::vector<vec3> gvertices() const
        {
            static constexpr auto delta = 0.01f;
            static constexpr auto deltax = vec3{ delta, 0.f, 0.f };
            static constexpr auto deltay = vec3{ 0.f, delta, 0.f };
            static constexpr auto deltaz = vec3{ 0.f, 0.f, delta };

            auto const& volpoints = beziermaths::tessellate(vol);
            std::vector<vec3> res;
            res.reserve(volpoints.size() * 2);
            for (auto const& v : volpoints)
            {
                res.push_back(v);
                res.push_back(v + deltax);
                res.push_back(v);
                res.push_back(v - deltax);
                res.push_back(v);
                res.push_back(v + deltay);
                res.push_back(v);
                res.push_back(v - deltay);
                res.push_back(v);
                res.push_back(v + deltaz);
                res.push_back(v);
                res.push_back(v - deltaz);
            }

            return res;
        }

        std::vector<gfx::instance_data> instancedata() const { return { gfx::instance_data(matrix::CreateTranslation(vec3::Zero), gfx::getview(), gfx::getmat("")) }; }

        static qbeziervolume create(float len)
        {
            qbeziervolume unitvol;
            for (auto i : std::ranges::iota_view{ 0u, unitvol.vol.numcontrolpts })
            {
                auto const& idx = stdx::hypercubeidx<2>::from1d(2, i);
                unitvol.vol.controlnet[i] = len * vec3{ static_cast<float>(idx.coords[0]) / 2, static_cast<float>(idx.coords[2]) / 2 , static_cast<float>(idx.coords[1]) / 2 };
            }

            return unitvol;
        }

        beziermaths::beziervolume<2u> vol;
    };
}