#include "GameUtils.h"

namespace game_creator
{
	template <game_types type>
	std::unique_ptr<game_base> create_instance(game_engine const* engine)
	{
		return std::move(std::make_unique<dummy_game>());
	}
}

dummy_game::dummy_game()
	: game_base(nullptr)
{}