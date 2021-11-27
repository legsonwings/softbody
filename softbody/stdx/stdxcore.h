#pragma once

#include <cstddef>
#include <concepts>
#include <type_traits>

using uint = std::size_t;

namespace stdx
{
template <typename t> concept uint_c = std::convertible_to<t, uint>;

template <typename t>
concept arithmetic_c = std::is_arithmetic_v<t>;

template <typename t>
concept lvaluereference_c = std::is_lvalue_reference_v<t>;

template <typename t>
concept notlvaluereference_c = !std::is_lvalue_reference_v<t>;

template <typename t>
concept arithmeticvector_c = requires(t v)
{
	v.x; v.y;
	requires arithmetic_c<decltype(v.x)>&& arithmetic_c<decltype(v.y)>;
};

template <typename t>
concept indexablecontainer_c = requires(t v)
{
	{v.size()} -> std::convertible_to<std::size_t>;
	v.operator[](std::size_t());
};
}