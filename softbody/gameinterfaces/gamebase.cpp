#include "gamebase.h"
#include "engine/graphics/gfxutils.h"
#include "directxmath.h"

game_base::game_base(gamedata const& data)
{
    camera.nearplane(data.nearplane);
    camera.farplane(data.farplane);
    camera.width(data.width);
    camera.height(data.height);
}

void game_base::updateview(float dt)
{
	camera.Update(dt);

    gfx::getview().view = camera.GetViewMatrix();
}