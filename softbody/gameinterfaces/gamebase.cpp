#include "gamebase.h"
#include "engine/graphics/gfxutils.h"
#include "directxmath.h"

game_base::game_base(gamedata const& data)
{
    camera.nearplane(data.nearplane);
    camera.farplane(data.farplane);
    camera.aspectratio(data.get_aspect_ratio());
}

void game_base::updateview(float dt)
{
	camera.Update(dt);

    gfx::getview().view = camera.GetViewMatrix();
    gfx::getview().proj = camera.GetProjectionMatrix(XM_PI / 3.0f);
}