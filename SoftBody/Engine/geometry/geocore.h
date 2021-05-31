#pragma once

#include "Engine/SimpleMath.h"

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

    struct aabb
    {
        aabb() = default;
        aabb(aabb const&) = default;
        aabb(aabb&&) = default;
        aabb& operator=(const aabb&) = default;
        aabb& operator=(aabb&&) = default;

        aabb(std::vector<vec3> const& points);
        constexpr aabb(vec3 const& _min, vec3 const& _max) : min_pt(_min), max_pt(_max) {}        

        vec3 span() const { return max_pt - min_pt; }

        // Top left back is min, bot right front max
        vec3 min_pt;
        vec3 max_pt;
    };
}