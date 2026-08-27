#include "Generator.hpp"
#include "TileInfo.hpp"
#include "../Timer.hpp"
#include "SaveManager.hpp"
#include "Utils.hpp"
#include "PerlinNoise.hpp"
namespace cc
{
	Generator::Generator()
	{
	}
	Chunk *Generator::GenerateChunk(sf::Vector2i position)
	{
		Chunk *c = new Chunk(position);

		siv::PerlinNoise perlin(seed);

		// Controls how "zoomed in" the noise is.
		// Larger = smoother, larger patches.
		const double noiseScale = 0.03;

		// Controls the range of brightness.
		const int minBrightness = 0;
		const int maxBrightness = 255;

		for (int y = 0; y < CHUNK_SIZE; y++)
		{
			for (int x = 0; x < CHUNK_SIZE; x++)
			{
				if (y == 0 || y == CHUNK_SIZE - 1 ||
					x == 0 || x == CHUNK_SIZE - 1)
				{
					c->tiles[x][y] = Tile(5);
				}

				// Convert chunk-local coordinates into world coordinates.
				int worldX = position.x * CHUNK_SIZE + x;
				int worldY = position.y * CHUNK_SIZE + y;

				// Perlin noise returns approximately [-1, 1].
				double noise = perlin.octave2D_01(
					worldX * noiseScale,
					worldY * noiseScale,
					4);

				// octave2D_01 returns [0, 1], so map it to our brightness range.
				int brightness =
					minBrightness +
					static_cast<int>(
						noise * (maxBrightness - minBrightness));
				brightness = std::clamp(brightness,10,240);
				c->backgroundTiles[x][y].color =
					sf::Color(brightness, brightness, brightness);
			}
		}

		return c;
	}

	void Generator::SetSeed(uint64_t seed)
	{
		this->seed = seed;
	}
	nlohmann::json Generator::ToJson()
	{
		nlohmann::json j;
		j["seed"] = seed;
		return j;
	}
	void Generator::FromJson(nlohmann::json j)
	{
		seed = j["seed"];
	}
}