#pragma once

#include "geodefines.h"
#include "Engine/SimpleMath.h"

#include <vector>
#include <optional>

namespace geometry
{
    struct nullshape {};

    struct vertex
    {
        vec3 position;
        vec3 normal;

        vertex() = default;
        constexpr vertex(vec3 const& pos, vec3 const& norm) : position(pos), normal(norm){}
    };

    struct box
    {
        box() = default;
        constexpr box(vec3 const& _center, vec3 const& _extents) : center(_center), extents(_extents) {}

        std::vector<vec3> get_vertices();
        vec3 center, extents;
    };

    struct aabb
    {
        aabb() = default;
        aabb(vec3 const *tri);

        aabb(std::vector<vec3> const& points);
        constexpr aabb(vec3 const& _min, vec3 const& _max) : min_pt(_min), max_pt(_max) {}

        vec3 center() const { return (max_pt + min_pt) / 2.f; }
        vec3 span() const { return max_pt - min_pt; }
        operator box() const { return { center(), span() }; }
        std::optional<aabb> intersect(aabb const& r) const;

        // Top left back is min, bot right front max
        vec3 min_pt;
        vec3 max_pt;
    };
}