#pragma once

#include "engine/engineutils.h"
#include "engine/graphics/gfxcore.h"
#include "engine/simplecamera.h"

// abstract base game class
class game_base
{
public:
	game_base(gamedata const &data);
	
	virtual ~game_base() {}

	virtual gfx::resourcelist load_assets_and_geometry() = 0;

	virtual void update(float dt) { updateview(dt); };
	virtual void render(float dt) = 0;

	virtual void on_key_down(unsigned key) { camera.OnKeyDown(key); };
	virtual void on_key_up(unsigned key) { camera.OnKeyUp(key); };

protected:
	SimpleCamera camera;

	void updateview(float dt);
};