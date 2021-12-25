#pragma once

#include <cstddef>
#include <concepts>
#include <type_traits>

using uint = std::size_t;

namespace stdx
{
template <typename t> concept uint_c = std::convertible_to<t, uint>;

template <typename t>
concept lvaluereference_c = std::is_lvalue_reference_v<t>;

template <typename t>
concept rvaluereference_c = std::is_rvalue_reference_v<t>;

template <typename t>
concept triviallycopyable_c = std::is_trivially_copyable_v<t>;

template <typename t>
concept arithmeticpure_c = std::is_arithmetic_v<t>;

template <typename t>
concept arithmetic_c = requires(t v)
{
	{-v}-> std::convertible_to<t>;
	{v* float()}-> std::convertible_to<t>;
	{v + v} -> std::convertible_to<t>;
	{v - v} -> std::convertible_to<t>;
	{v / float()} -> std::convertible_to<t>;
};

template <typename t>
concept indexablecontainer_c = requires(t v)
{
	{v.size()} -> std::convertible_to<std::size_t>;
	v.operator[](std::size_t());
};

template <typename t, typename u>
concept samedecay_c = std::same_as<std::decay_t<t>, std::decay_t<u>>;

}