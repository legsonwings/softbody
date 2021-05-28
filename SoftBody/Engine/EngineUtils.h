#pragma once

#include <string>
#include <limits>

class game_engine;

using uint = std::size_t;

struct configurable_properties
{
	unsigned width = 1280;
	unsigned height = 720;
	static constexpr unsigned frame_count = 2;

	std::wstring app_name = L"Soft Body Engine";

	float get_aspect_ratio() const
	{
		return static_cast<float>(width) / static_cast<float>(height);
	}
};

namespace utils
{
	template <typename T>
	T constexpr tolerance = T(1e-4f);

	// todo : error for non-numeric types including char, maybe use concepts
	template <typename T>
	struct invalid
	{
		constexpr operator T() const { return std::numeric_limits<T>::max(); }
	};

	template <typename T>
	bool constexpr isvalid(T const& val) { return std::numeric_limits<T>::max() - val >= tolerance<T>; }
}