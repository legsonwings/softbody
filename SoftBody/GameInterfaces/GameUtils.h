#pragma once

enum class game_types
{
	soft_body_demo
};

auto constexpr currrent_game = game_types::soft_body_demo;

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
	std::unique_ptr<game_base> create_instance<game_types::soft_body_demo>(game_engine const* engine);
}