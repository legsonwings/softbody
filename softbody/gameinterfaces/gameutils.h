#pragma once

#include <string>

enum class game_types : int
{
	softbodydemo,
	fluidsimulation,
	num
};

extern std::wstring gametitles[int(game_types::num)];
auto constexpr currentgame = game_types::softbodydemo;

#include "gamebase.h"

class dummy_game : public game_base
{
public:
	dummy_game();

	void update(float dt) override {};
	void render(float dt) override {};
};

class game_engine;

namespace game_creator
{
	template <game_types type>
	std::unique_ptr<game_base> create_instance(gamedata const& data);

	template <>
	std::unique_ptr<game_base> create_instance<currentgame>(gamedata const& data);
}