#include "Atlas.hpp"
#include "SaveManager.hpp"
namespace cc
{
	Atlas::Atlas(std::vector<sf::Texture> &textures)
	{
		positions = {};
		// algorithm that finds the position of each texture. Added in rows with the height of the maximum height of the texture.
		for (int i = 0; i < textures.size(); i++)
		{
			AddTexture(textures[i]);
		}
		Build();
	}
	Atlas::Atlas()
	{
	}
	void Atlas::AddTexture(sf::Texture texture)
	{
		// increase width by width of texture
		int newWidth = rowWidth + texture.getSize().x;
		// if too big, move on to new row
		if (newWidth > maxSize)
		{
			totalHeight += rowHeight;
			rowHeight = 0;
			rowWidth = 0;
		}
		// if width of row is greater than width of texture, expand width of texture
		if (newWidth > totalWidth)
		{
			totalWidth = newWidth;
		}
		// if height greater than row height, increase row height
		if (texture.getSize().y > rowHeight)
		{
			rowHeight = texture.getSize().y;
		}
		// add position and expand size of row
		positions.push_back(sf::Vector2i(rowWidth, totalHeight));
		rowWidth += texture.getSize().x;
		textures.push_back(texture);
	}
	void Atlas::Build()
	{
		// add height of final row
		totalHeight += rowHeight;
		sf::Image im(sf::Vector2u(totalWidth, totalHeight));
		// create image and add textures in positions calculated
		for (int i = 0; i < textures.size(); i++)
		{
			auto i2 = textures[i].copyToImage();
			if (!im.copy(i2, {(unsigned int)positions[i].x, (unsigned int)positions[i].y}))
			{
				std::cerr << "failed to copy image to atlas";
			}
		}
		// convert to texture
		if (!texture.loadFromImage(im))
		{
			std::cerr << "failed to convert to texture";
		}
		// texture.copyToImage().saveToFile(SaveManager::GetSavedataDir() + "/atlas.png");
	}
}