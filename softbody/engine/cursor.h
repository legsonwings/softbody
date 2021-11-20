#pragma once

#include "engine/core.h"

class cursor
{
public:
	cursor() : _pos(pos()), _lastpos(pos()) { };

	void tick(float dt);
	vector2 pos() const;
	vector2 vel() const;
	vector3 ray(float nearp, float farp) const;
	vector3 to3d(vector3 pos, float nearp, float farp) const;

private:
	vector2 _lastpos = {};
	vector2 _pos = {};
	vector2 _vel = {};
};

