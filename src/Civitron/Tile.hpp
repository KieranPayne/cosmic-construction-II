#pragma once
#include "../PCH.hpp"

namespace Civitron
{
#define TILE_SIZE 16
	struct Tile
	{
	public:
		uint16_t type;
		Tile() : type(0)
		{
		}
		Tile(uint16_t type){this->type  = type;}
	};
}