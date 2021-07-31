#pragma once

#include "geocore.h"
#include "engine/engineutils.h"
#include "engine/graphics/gfxcore.h"

namespace geometry
{
    struct cube
    {
        cube() = default;
        constexpr cube(vec3 const& _center, vec3 const& _extents) : center(_center), extents(_extents) {}

        std::vector<geometry::vertex> get_vertices() const;
        std::vector<geometry::vertex> get_vertices_invertednormals() const;
        std::vector<gfx::instance_data> getinstancedata() const;
        vec3 center, extents;
    };
}