#pragma once

#include "engine/geometry/geodefines.h"

class cursor
{
public:
	cursor() : _pos(pos()), _lastpos(pos()) { };

	void tick(float dt);
	vec2 pos() const;
	vec2 vel() const;
	vec3 ray() const;
	vec3 to3d(vec3 screenpos) const;

private:
	vec2 _lastpos = {};
	vec2 _pos = {};
	vec2 _vel = {};
};

