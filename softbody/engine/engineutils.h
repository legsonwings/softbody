#pragma once

#include <random>

class game_engine;

struct gamedata
{
	unsigned width = 720;
	unsigned height = 720;
	float nearplane = 0.1f;
	float farplane = 1000.f;

	float get_aspect_ratio() const { return static_cast<float>(width) / static_cast<float>(height); }
};

struct configurable_properties
{
	static constexpr unsigned frame_count = 2;
};

namespace engineutils
{
	std::mt19937& getrandomengine();
}