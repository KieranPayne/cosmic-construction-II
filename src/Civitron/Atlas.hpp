#pragma once
#include "../PCH.hpp"
namespace Civitron
{

	class Atlas
	{
	public:
		sf::Texture texture;
		int rowHeight = 0;
		int rowWidth = 0;
		int totalWidth = 0;
		int totalHeight = 0;
		int maxSize = sf::Texture::getMaximumSize();
		std::vector<sf::Texture> textures;
		std::vector<sf::Vector2i> positions;
		
		Atlas(std::vector<sf::Texture> &textures);
		Atlas();
		void AddTexture(sf::Texture texture);
		void Build();
	};
}