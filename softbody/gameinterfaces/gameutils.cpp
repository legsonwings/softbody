#include "gameutils.h"

std::wstring gametitles[int(game_types::num)] = { L"Soft Body Demo", L"Fluid simulation" };

namespace game_creator
{
	template <game_types type>
	std::unique_ptr<game_base> create_instance(gamedata const&)
	{
		return std::move(std::make_unique<dummy_game>());
	}
}

dummy_game::dummy_game() : game_base(gamedata{})
{}