#pragma once

#include "stdx/stdxcore.h"
#include "stdx/stdx.h"

#include <array>
#include <functional>

namespace stdx
{
template<uint d, stdx::arithmetic_c t = float>
struct vec : public std::array<t, d + 1>
{
	static constexpr uint nd = d;
	constexpr vec operator-() const { return stdx::unaryop(*this, stdx::uminus()); }
	constexpr vec operator+(vec r) const { return stdx::binaryop(*this, r, std::plus<>()); }
	constexpr vec operator-(vec r) const { return stdx::binaryop(*this, r, std::minus<>()); }
	constexpr vec operator/(t r) const { return stdx::unaryop(*this, std::bind(std::multiplies<>(), std::placeholders::_1, t(1) / r)); }

	constexpr operator t() const requires (d == 0) { return (*this)[0]; }

	template<typename d_t>
	constexpr vec<d, d_t> castas() const { return { stdx::castas<d_t>(*this) }; }
};

template<stdx::arithmetic_c t, uint d>
vec<d, t> operator*(vec<d, t> l, t r) { return { stdx::unaryop(l, std::bind(std::multiplies<>(), std::placeholders::_1, r)) }; }

template<stdx::arithmetic_c t, uint d>
vec<d, t> operator*(t l, vec<d, t> r) { return { stdx::unaryop(r, std::bind(std::multiplies<>(), std::placeholders::_1, l)) }; }

template<uint d>
using veci = vec<d, int>;

template<uint d>
using vecui = vec<d, uint>;

using vec1 = vec<0>;
using vec2 = vec<1>;
using vec3 = vec<2>;

using veci1 = veci<0>;
using veci2 = veci<1>;
using veci3 = veci<2>;

using vecui1 = vecui<0>;
using vecui2 = vecui<1>;
using vecui3 = vecui<2>;
}