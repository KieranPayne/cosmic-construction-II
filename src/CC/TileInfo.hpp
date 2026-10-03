#pragma once
#include "../PCH.hpp"
#include "Atlas.hpp"
namespace cc
{
	uint16_t GetTileID(std::string name);
	namespace TileInfo
	{
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

		struct TileData
		{
			// std::string path;
			std::string name;
			std::vector<sf::Vector2i> positions;
			std::vector<sf::Image> images;
			std::vector<std::string> paths;
			uint16_t id;
			bool isTileEntity;
		};
		extern Atlas atlas;
		extern std::vector<TileData> tileRegistry;
		extern std::unordered_map<std::string, TileData *, StringHash> nameLookup;
		void Init();
	}
}