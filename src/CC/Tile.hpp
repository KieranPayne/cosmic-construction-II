#pragma once
#include "../PCH.hpp"

namespace cc
{
#define TILE_SIZE 16
	struct Tile
	{
	public:
		uint16_t type;
		Tile() : type(0)
		{
		}
		Tile(uint16_t type) { this->type = type; }
	};
	enum class BackgroundTileType : uint8_t
	{
		WATER = 0,
		SAND,
		GRASS,
		STONE
	};
	struct BackgroundTile
	{
	public:
		sf::Color color;
		BackgroundTileType type;
	};
}