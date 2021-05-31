#pragma once

#include <string>

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