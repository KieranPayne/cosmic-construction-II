#include "Chunk.hpp"
#include "TileInfo.hpp"
namespace Civitron
{
	Chunk::Chunk(sf::Vector3i position)
	{
		this->position = position;
		for (int x = 0; x < CHUNK_SIZE; x++)
		{
			for (int y = 0; y < CHUNK_SIZE; y++)
			{
				for (int z = 0; z < CHUNK_SIZE; z++)
				{
					tiles[x][y][z].type = 0;
				}
			}
		}
	}
	Chunk::Chunk()
	{
		position = {0, 0, 0};
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
		std::array<uint8_t, CHUNK_NUM_BYTES> values;
		for (int x = 0; x < CHUNK_SIZE; x++)
		{
			for (int y = 0; y < CHUNK_SIZE; y++)
			{
				for (int z = 0; z < CHUNK_SIZE; z++)
				{
					int index = (x * CHUNK_SIZE * CHUNK_SIZE + y * CHUNK_SIZE + z) * 2;
					values[index] = tiles[x][y][z].type >> 8;
					values[index + 1] = tiles[x][y][z].type & 255;
				}
			}
		}
		return values;
	}
	void Chunk::FromBytes(std::array<uint8_t,CHUNK_NUM_BYTES>bytes)
	{
		constexpr int numBytes = CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE * 2;
		for (int x = 0; x < CHUNK_SIZE; x++)
		{
			for (int y = 0; y < CHUNK_SIZE; y++)
			{
				for (int z = 0; z < CHUNK_SIZE; z++)
				{
					int i = x * CHUNK_SIZE * CHUNK_SIZE + y * CHUNK_SIZE + z;
					i *= 2;
					uint16_t value = (uint16_t)(bytes[i] << 8) + bytes[i + 1];
					tiles[x][y][z].type = value;
				}
			}
		}
	}
	void Chunk::RemoveEntity(Entity *entity)
	{
		for (int i = 0; i < entities.size(); i++)
		{
			if (entities[i] == entity)
			{
				entities.erase(entities.begin() + i);
				break;
			}
		}
	}

}