#pragma once

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
            std::vector<vec3> result;
            for (auto const& v : beziermaths::tessellate<2u>(curve))
                result.push_back(v.first);

            return result;
        }

        std::vector<gfx::instance_data> instancedata() const { return { gfx::instance_data(matrix::CreateTranslation(vec3::Zero), gfx::getview(), gfx::getmat("")) }; }

        beziermaths::beziercurve<2u> curve;
    };
}