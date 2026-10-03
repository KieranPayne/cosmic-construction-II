#pragma once
#include "../PCH.hpp"
#include "Atlas.hpp"
namespace cc
{
	/**
	 * @brief Looks up a tile type's id by its name.
	 * @param name The tile's name as written in TileInfo.json (for example "Air").
	 * @return The tile's id. The name must exist.
	 */
	uint16_t GetTileID(std::string name);

	/**
	 * @brief Information about every kind of tile, loaded from content/resources/TileInfo.json.
	 */
	namespace TileInfo
	{
		/// Hashes a string (32-bit FNV-1a), for use as the hash of `nameLookup`.
		struct StringHash
		{
			std::size_t operator()(const std::string &str) const
			{
				std::size_t hash = 2166136261u;

				for (char c : str)
				{
					hash ^= static_cast<unsigned char>(c);
					hash *= 16777619u;
				}

				return hash;
			}
		};

		/// Everything known about one kind of tile.
		struct TileData
		{
			/// The tile's name.
			std::string name;

			/// Where each of the tile's textures is in `atlas`, in pixels. Same order as `paths`.
			std::vector<sf::Vector2i> positions;

			/// The loaded image of each texture. Same order as `paths`.
			std::vector<sf::Image> images;

			/// The file each texture was loaded from. Same order as `images`.
			std::vector<std::string> paths;

			/// The tile's type id: its index in `tileRegistry`.
			uint16_t id;

			/// Whether tiles of this type have a TileEntity.
			bool isTileEntity;
		};

		/// All tile textures packed into one image. Tiles are drawn using this as their texture.
		extern Atlas atlas;

		/// Every kind of tile, indexed by tile type id.
		extern std::vector<TileData> tileRegistry;

		/// Finds a tile's data by name. The pointers point into `tileRegistry`, so they stay valid only as long as it isn't resized.
		extern std::unordered_map<std::string, TileData *, StringHash> nameLookup;

		/// @brief Loads TileInfo.json and every tile image, fills `tileRegistry` and `nameLookup`, and builds `atlas`. Call once at startup.
		void Init();
	}
}