#include "TileInfo.hpp"
#include "Utils.hpp"
#include <fstream>
#include <iostream>
#include "../json.hpp"
namespace cc
{
	uint16_t GetTileID(std::string name)
	{
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
			std::ifstream file("content/resources/TileInfo.json");
			nlohmann::json data = nlohmann::json::parse(file);
			file.close();
			int i = 0;
			for (auto &j : data)
			{
				TileData tileData;
				tileData.id = i;
				tileData.name = j["name"];
				tileData.isTileEntity = j["isTileEntity"];
				tileData.paths = {};
				for (auto &j2 : j["textures"])
				{
					tileData.paths.push_back("content/resources/images/" + (std::string)(j2));
					sf::Image im;
					im.loadFromFile(tileData.paths.back());
					tileData.images.push_back(im);
				}
				tileRegistry.push_back(tileData);
				i++;
			}
			for (auto &tile : tileRegistry)
			{
				nameLookup[tile.name] = &tile;
			}
			// build the atlas from every tile image
			std::vector<sf::Texture> textures;
			for (auto &t : tileRegistry)
			{
				for (int j = 0; j < t.paths.size(); j++)
				{
					sf::Image i;
					if (!i.loadFromFile(t.paths[j]))
					{
						std::cout << "failed to build atlas" << std::endl;
					}
					sf::Texture texture;
					if (!texture.loadFromImage(i))
					{
						std::cout << "failed to build atlas" << std::endl;
					}
					textures.push_back(texture);
				}
			}
			atlas = Atlas(textures);
			int index = 0;
			for (int i = 0; i < tileRegistry.size(); i++)
			{
				for (int j = 0; j < tileRegistry[i].paths.size(); j++)
				{
					tileRegistry[i].positions.push_back(atlas.positions[index]);
					index++;
				}
			}
		}
	}
}