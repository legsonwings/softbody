#pragma once

enum class game_types
{
	softbodydemo,
	fluidsimulation
};

auto constexpr current_game = game_types::fluidsimulation;

#include "GameBase.h"

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
	std::unique_ptr<game_base> create_instance(game_engine const * engine);

	template <>
	std::unique_ptr<game_base> create_instance<current_game>(game_engine const* engine);
}