#include "Generator.hpp"
#include "TileInfo.hpp"
#include "../Timer.hpp"
#include "SaveManager.hpp"
#include "Utils.hpp"
namespace cc
{
	Generator::Generator()
	{
	}
	Chunk *Generator::GenerateChunk(sf::Vector2i position)
	{
		Chunk* c = new Chunk(position);
		for (int y = 0; y < CHUNK_SIZE; y ++)
		{
			for (int x = 0; x < CHUNK_SIZE; x ++)
			{
				// c->tiles[x][y] = Tile(rand() % 7);
				if (y == 0 || y == CHUNK_SIZE - 1 || x == 0 || x == CHUNK_SIZE - 1)
				{
					c->tiles[x][y] = Tile(5);
				}
			}
		}
		return c;
	}
}