#include "TileInfo.hpp"
#include "Utils.hpp"
#include <fstream>
#include <iostream>
namespace cc
{
	uint16_t GetTileID(std::string name){
		return TileInfo::nameLookup[name]->id;
	}
	namespace TileInfo
	{
		bool ImageIsSolid(sf::Image &im)
		{
			for (unsigned int x = 0; x < im.getSize().x; x++)
			{
				for (unsigned int y = 0; y < im.getSize().y; y++)
				{
					if (im.getPixel({x, y}).a < 255)
					{
						return false;
					}
				}
			}
			return true;
		}
		std::vector<TileData> tileRegistry = {};
		std::unordered_map<std::string, TileData *, StringHash> nameLookup;
		Atlas atlas;
		void Init()
		{
			std::ifstream file("content/resources/TileInfo.txt");
			std::string line;
			// skip headers
			std::getline(file, line);
			while (std::getline(file, line))
			{
				auto result = Split(line, '|');
				TileData data;
				data.name = result[0];
				data.isTileEntity = result[2] == "true";
				// data.color = HexToColor(result[1]);
				data.path = "content/resources/images/" + result[1];
				data.id = (uint16_t)(tileRegistry.size());
				tileRegistry.push_back(data);
			}
			file.close();
			for (auto &tile : tileRegistry)
			{
				nameLookup[tile.name] = &tile;
			}
			// set up atlas
			std::vector<sf::Texture> textures;
			for (auto &t : tileRegistry)
			{
				sf::Image i;
				if (!i.loadFromFile(t.path))
				{
					std::cout << "failed to build atlas" << std::endl;
				}
				// t.solid = ImageIsSolid(i);
				sf::Texture texture;
				if (!texture.loadFromImage(i))
				{
					std::cout << "failed to build atlas" << std::endl;
				}
				textures.push_back(texture);
			}
			atlas = Atlas(textures);
			for (int i = 0; i < tileRegistry.size(); i++)
			{
				tileRegistry[i].position = atlas.positions[i];
			}
		}
	}

}