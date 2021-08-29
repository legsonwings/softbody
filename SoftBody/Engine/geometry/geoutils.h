#pragma once

#include "geocore.h"
#include "Engine/SimpleMath.h"

#include <concepts>

namespace geoutils
{
	template <typename t>
	concept arithmetic_c = std::is_arithmetic_v<t>;

	template <typename t>
	concept arithmeticvector_c = requires(t v)
	{
		v.x; v.y;
		requires arithmetic_c<decltype(v.x)>&& arithmetic_c<decltype(v.y)>;
	};

	template <typename t = float>
	requires arithmetic_c<t> || arithmeticvector_c<t>
	t constexpr tolerance = t(1e-4f);

	template <typename t>
	requires arithmetic_c<t> || arithmeticvector_c<t>
	struct invalid
	{
		constexpr operator t() const { return { std::numeric_limits<t>::max() }; }
	};

	template <typename t>
	requires arithmeticvector_c<t>
	struct arithmeticvector {};

    vec4 create_vec4(vec3 const& v, float w = 1.f);
    matrix get_planematrix(vec3 const &translation, vec3 const &normal);
	std::vector<geometry::vertex> create_cube(vec3 const& center, vec3 const& extents);
	std::vector<vec3> create_box_lines(vec3 const &center, vec3 const& extents);
	std::vector<vec3> create_cube_lines(vec3 const &center, float scale);
	bool nearlyequal(arithmetic_c auto const& l, arithmetic_c auto const& r, float const& _tolerance = tolerance<float>);
	bool nearlyequal(vec2 const& l, vec2 const& r, float const& _tolerance = tolerance<float>);
	bool nearlyequal(vec3 const& l, vec3 const& r, float const& _tolerance = tolerance<float>);
	bool nearlyequal(vec4 const& l, vec4 const& r, float const& _tolerance = tolerance<float>);

	template <typename t>
	requires arithmetic_c<t> || arithmeticvector_c<t>
	bool constexpr isvalid(t const& val) { return !nearlyequal(std::numeric_limits<t>::max(), val); }
}

namespace std
{
	template <typename t>
	class numeric_limits<geoutils::arithmeticvector<t>>
	{
		static constexpr t min() noexcept { return t(std::numeric_limits<decltype(t().x)>::min()); }
		static constexpr t max() noexcept { return t(std::numeric_limits<decltype(t().x)>::max()); }
		static constexpr t lowest() noexcept { return t(std::numeric_limits<decltype(t().x)>::lowest()); }
	};
}