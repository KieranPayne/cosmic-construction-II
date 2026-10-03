#pragma once
#include "../PCH.hpp"

namespace cc
{
	/// Width and height of one tile, in pixels.
#define TILE_SIZE 16

	/**
	 * @brief A tile in a chunk's main layer: just its type.
	 */
	struct Tile
	{
	public:
		/// Which tile this is: an index into TileInfo::tileRegistry. 0 is air (empty).
		uint16_t type;

		/// @brief Creates an air tile.
		Tile() : type(0)
		{
		}

		/// @brief Creates a tile of the given type.
		Tile(uint16_t type) { this->type = type; }
	};

	/**
	 * @brief The terrain a background tile is made of, chosen by the generator.
	 */
	enum class BackgroundTileType : uint8_t
	{
		WATER = 0,
		WATERY_SAND,
		SAND,
		GRASS,
		GRASSY_STONE,
		STONE,
		TALL_STONE
	};

	/**
	 * @brief A tile in a chunk's background layer, which sits behind the main tiles.
	 */
	struct BackgroundTile
	{
	public:
		/// The colour this tile is drawn in.
		sf::Color color;

		/// The terrain type this tile was generated as.
		BackgroundTileType type;
	};
}