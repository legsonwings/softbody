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

template<typename b, typename e>
struct ext
{
	b base;
	e ext;

	constexpr ext(b const& _b, e const& _e) : base(_b), ext(_e) {}

	constexpr operator b() { return base; }
	constexpr operator b const() const { return base; }
	constexpr e &ex() { return ext; }
	constexpr e const &ex() const { return ext; }
	constexpr b &get() { return base; }
	constexpr b const &get() const { return base; }
	constexpr b &operator*() { return base; }
	constexpr b const &operator*() const { return base; }
	constexpr b *operator->() { return &base; }
	constexpr b const *operator->() const { return &base; }
};