#pragma once

#include <string>
#include <random>

class game_engine;

struct configurable_properties
{
	unsigned width = 1280;
	unsigned height = 720;
	static constexpr unsigned frame_count = 2;

	std::wstring app_name = L"Soft Body Demo";

	float get_aspect_ratio() const { return static_cast<float>(width) / static_cast<float>(height); }
};

namespace engineutils
{
	std::mt19937& getrandomengine();
}