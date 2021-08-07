#include "stdafx.h"
#include "engineutils.h"

std::random_device rd;
std::mt19937& engineutils::getrandomengine()
{
	static std::mt19937 mt(rd());

	return mt;
}
