#pragma once

#include <string>
#include <array>
#include <vector>
#include <memory>

#include "Engine/Interfaces/BodyInterface.h"

class game_engine;

namespace gfx
{
	class body;
}

// abstract base game class
class game_base
{
public:
	game_base(game_engine const * _engine);
	
	virtual ~game_base() {}

	virtual std::vector<std::weak_ptr<gfx::body>> load_assets_and_geometry() = 0;

	virtual void update(float dt) = 0;
	virtual void render(float dt) = 0;
	virtual void populate_command_list() = 0;

	virtual void on_key_down(unsigned key) {};
	virtual void on_key_up(unsigned key) {};

protected:
	game_engine const * engine;
};