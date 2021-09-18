#pragma once

#include "geocore.h"
#include "engine/stdx.h"
#include "engine/SimpleMath.h"

#include <concepts>

namespace geoutils
{
    vec4 create_vec4(vec3 const& v, float w = 1.f);
    matrix get_planematrix(vec3 const &translation, vec3 const &normal);
	std::vector<geometry::vertex> create_cube(vec3 const& center, vec3 const& extents);
	std::vector<vec3> create_box_lines(vec3 const &center, vec3 const& extents);
	std::vector<vec3> create_cube_lines(vec3 const &center, float scale);
	bool nearlyequal(stdx::arithmetic_c auto const& l, stdx::arithmetic_c auto const& r, float const& _tolerance = stdx::tolerance<float>);
	bool nearlyequal(vec2 const& l, vec2 const& r, float const& _tolerance = stdx::tolerance<float>);
	bool nearlyequal(vec3 const& l, vec3 const& r, float const& _tolerance = stdx::tolerance<float>);
	bool nearlyequal(vec4 const& l, vec4 const& r, float const& _tolerance = stdx::tolerance<float>);
}