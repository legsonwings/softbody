#pragma once

#include "geocore.h"
#include "stdx/stdx.h"
#include "engine/SimpleMath.h"

#include <concepts>

namespace geoutils
{
    vector4 create_vector4(vector3 const& v, float w = 1.f);
    matrix get_planematrix(vector3 const &translation, vector3 const &normal);
	std::vector<geometry::vertex> create_cube(vector3 const& center, vector3 const& extents);
	std::vector<vector3> create_box_lines(vector3 const &center, vector3 const& extents);
	std::vector<vector3> create_cube_lines(vector3 const &center, float scale);
	bool nearlyequal(stdx::arithmeticpure_c auto const& l, stdx::arithmeticpure_c auto const& r, float const& _tolerance = stdx::tolerance<float>);
	bool nearlyequal(vector2 const& l, vector2 const& r, float const& _tolerance = stdx::tolerance<float>);
	bool nearlyequal(vector3 const& l, vector3 const& r, float const& _tolerance = stdx::tolerance<float>);
	bool nearlyequal(vector4 const& l, vector4 const& r, float const& _tolerance = stdx::tolerance<float>);
}