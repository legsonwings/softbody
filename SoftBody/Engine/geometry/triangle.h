#pragma once

#include "geocore.h"
#include "line.h"
#include "Engine/EngineUtils.h"

#include <optional>

namespace geometry
{
    struct triangle2D
    {
        triangle2D() = default;
        constexpr triangle2D(vec2 const& _v0, vec2 const& _v1, vec2 const& _v2) : v0(_v0), v1(_v1), v2(_v2) {}

        bool isin(vec2 const& point) const;

        vec2 v0, v1, v2;
    };

    struct triangle
    {
        triangle() = default;
        constexpr triangle(vec3 const& _v0, vec3 const& _v1, vec3 const& _v2) : verts{ _v0, _v1, _v2 } {}

        operator plane() const { return { verts[0], verts[1], verts[1] }; }

        static bool isin(vec3 const* tri, vec3 const& point);
        static std::optional<linesegment> intersect(triangle const& t0, triangle const& t1);
        static std::optional<linesegment> intersect(vec3 const* t0, vec3 const* t1);

        vec3 verts[3];
    };
}