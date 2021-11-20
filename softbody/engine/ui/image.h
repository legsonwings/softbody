#pragma once

#include "engine/ui/uicore.h"
#include "engine/graphics/gfxcore.h"

namespace ui
{
class image
{
	canvasslot _slot;
	gfx::resourcelist createresources();
};
}
