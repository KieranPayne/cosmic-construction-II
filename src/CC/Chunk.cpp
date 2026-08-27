#include "Chunk.hpp"
#include "TileInfo.hpp"
namespace cc
{
	Chunk::Chunk(sf::Vector2i position)
	{
		this->position = position;
		for (int x = 0; x < CHUNK_SIZE; x++)
		{
			for (int y = 0; y < CHUNK_SIZE; y++)
			{
				tiles[x][y].type = 0;
				backgroundTiles[x][y].color = sf::Color::White;
				// int brightness = rand() % 256;
				// backgroundTiles[x][y].color = sf::Color(brightness,brightness,brightness);
			}
		}
	}
	Chunk::Chunk()
	{
		position = {0, 0};
	}

	sf::Vector2i closestSquareDims(int area)
	{
		int side = static_cast<int>(std::sqrt(area));

		for (int w = side; w > 0; --w)
		{
			if (area % w == 0)
			{
				int h = area / w;
				return {w, h}; // width and height
			}
		}

		return {1, area}; // Fallback, area is a prime number
	}

	std::array<uint8_t, CHUNK_NUM_BYTES> Chunk::ToBytes()
	{
		std::array<uint8_t, CHUNK_NUM_BYTES> bytes{};

		size_t index = 0;

		for (int x = 0; x < CHUNK_SIZE; x++)
		{
			for (int y = 0; y < CHUNK_SIZE; y++)
			{
				uint16_t type = tiles[x][y].type;

				bytes[index++] = static_cast<uint8_t>(type >> 8);
				bytes[index++] = static_cast<uint8_t>(type & 0xFF);

				bytes[index++] = backgroundTiles[x][y].color.r;
				bytes[index++] = backgroundTiles[x][y].color.g;
				bytes[index++] = backgroundTiles[x][y].color.b;
			}
		}

		return bytes;
	}

	void Chunk::FromBytes(
		std::array<uint8_t, CHUNK_NUM_BYTES> &bytes)
	{
		size_t index = 0;

		for (int x = 0; x < CHUNK_SIZE; x++)
		{
			for (int y = 0; y < CHUNK_SIZE; y++)
			{
				uint16_t type =
					(static_cast<uint16_t>(bytes[index]) << 8) |
					static_cast<uint16_t>(bytes[index + 1]);

				tiles[x][y].type = type;
				index += 2;

				backgroundTiles[x][y].color = sf::Color(
					bytes[index],
					bytes[index + 1],
					bytes[index + 2]);

				index += 3;
			}
		}
	}

}