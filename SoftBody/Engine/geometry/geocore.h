#pragma once

#include "Engine/SimpleMath.h"

#include <vector>
#include <optional>

using vec2 = DirectX::SimpleMath::Vector2;
using vec3 = DirectX::SimpleMath::Vector3;
using vec4 = DirectX::SimpleMath::Vector4;
using matrix = DirectX::SimpleMath::Matrix;
using plane = DirectX::SimpleMath::Plane;

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
        aabb(aabb const&) = default;
        aabb(aabb&&) = default;
        aabb& operator=(const aabb&) = default;
        aabb& operator=(aabb&&) = default;

        aabb(std::vector<vec3> const& points);
        constexpr aabb(vec3 const& _min, vec3 const& _max) : min_pt(_min), max_pt(_max) {}

        vec3 span() const { return max_pt - min_pt; }
        operator box() const { return { (min_pt + max_pt) / 2.f, max_pt - min_pt }; }
        std::optional<aabb> intersect(aabb const& r) const;

        // Top left back is min, bot right front max
        vec3 min_pt;
        vec3 max_pt;
    };
}