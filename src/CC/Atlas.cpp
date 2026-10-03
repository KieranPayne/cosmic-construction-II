#include "Atlas.hpp"
#include "SaveManager.hpp"
namespace cc
{
	// Pixels of duplicated edge colour added around every texture, so that rounding errors when
	// sampling next to a texture's edge never pick up a neighbouring texture.
	constexpr int PADDING = 1;

	Atlas::Atlas()
	{
		// A 1x1 white pixel at the start of the atlas: vertices without texture coordinates
		// sample it, so they are drawn with just their vertex colour.
		sf::Image whitePixel({1, 1}, sf::Color::White);
		sf::Texture whiteTexture;
		whiteTexture.loadFromImage(whitePixel);
		AddTexture(whiteTexture);
	}

	Atlas::Atlas(std::vector<sf::Texture> &initialTextures) : Atlas()
	{
		for (const sf::Texture &t : initialTextures)
		{
			AddTexture(t);
		}
		Build();
	}

	void Atlas::AddTexture(sf::Texture texture)
	{
		// each texture occupies its size plus a border on every side
		int w = texture.getSize().x + 2 * PADDING;
		int h = texture.getSize().y + 2 * PADDING;

		// start a new row if this texture would not fit in the current one
		if (rowWidth + w > maxSize)
		{
			totalHeight += rowHeight;
			rowHeight = 0;
			rowWidth = 0;
		}

		// the position is the top-left of the texture's real pixels, not of its border
		positions.push_back(sf::Vector2i(rowWidth + PADDING, totalHeight + PADDING));

		rowWidth += w;
		if (rowWidth > totalWidth)
			totalWidth = rowWidth;
		if (h > rowHeight)
			rowHeight = h;

		textures.push_back(texture);
	}

	void Atlas::Build()
	{
		// include the height of the final row
		totalHeight += rowHeight;
		sf::Image atlasImage(sf::Vector2u(totalWidth, totalHeight));

		for (std::size_t i = 0; i < textures.size(); i++)
		{
			sf::Image src = textures[i].copyToImage();
			int srcWidth = (int)src.getSize().x;
			int srcHeight = (int)src.getSize().y;
			int originX = positions[i].x;
			int originY = positions[i].y;

			if (!atlasImage.copy(src, {(unsigned)originX, (unsigned)originY}))
				std::cerr << "failed to copy image to atlas\n";

			// fill the border: every pixel outside the texture copies its nearest edge pixel
			for (int py = -PADDING; py < srcHeight + PADDING; py++)
			{
				for (int px = -PADDING; px < srcWidth + PADDING; px++)
				{
					bool insideTexture = px >= 0 && px < srcWidth && py >= 0 && py < srcHeight;
					if (insideTexture)
						continue;
					int sourceX = std::clamp(px, 0, srcWidth - 1);
					int sourceY = std::clamp(py, 0, srcHeight - 1);
					atlasImage.setPixel({(unsigned)(originX + px), (unsigned)(originY + py)},
										src.getPixel({(unsigned)sourceX, (unsigned)sourceY}));
				}
			}
		}

		if (!texture.loadFromImage(atlasImage))
			std::cerr << "failed to convert atlas image to texture\n";

		// remove the white pixel entry so indices match the order textures were added
		textures.erase(textures.begin());
		positions.erase(positions.begin());

		// write the atlas to disk so it can be inspected
		texture.copyToImage().saveToFile(SaveManager::GetSavedataDir() + "/atlas.png");
	}
}