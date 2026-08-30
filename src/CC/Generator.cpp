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
	// Helper function: Deterministic 2D hash to generate a offset between [-maxOffset, maxOffset]
	int GetTileOffset(int x, int y, uint32_t seed, int maxOffset = 15)
	{
		// Combine coordinates and world seed into a single hash value
		uint32_t hash = static_cast<uint32_t>(x) * 73856093u ^ static_cast<uint32_t>(y) * 19349663u ^ seed * 83492791u;

		// Scramble bits (Murmur3-style finalizer)
		hash ^= hash >> 16;
		hash *= 0x85ebca6b;
		hash ^= hash >> 13;
		hash *= 0xc2b2ae35;
		hash ^= hash >> 16;

		// Map to [-maxOffset, maxOffset] range
		int range = (maxOffset * 2) + 1;
		return (static_cast<int>(hash % range)) - maxOffset;
	}

	Chunk *Generator::GenerateChunk(sf::Vector2i position)
	{
		Chunk *c = new Chunk(position);
		siv::PerlinNoise perlin(seed);

		const double noiseScale = 0.03;

		// Intensity of color variation (+/- RGB value shift)
		const int colorVariation = 3;

		for (int y = 0; y < CHUNK_SIZE; y++)
		{
			for (int x = 0; x < CHUNK_SIZE; x++)
			{
				int worldX = position.x * CHUNK_SIZE + x;
				int worldY = position.y * CHUNK_SIZE + y;

				double noise = perlin.octave2D_01(
					worldX * noiseScale,
					worldY * noiseScale,
					4);

				int numColours = 4;
				sf::Color colours[numColours] = {
					{51, 128, 176},
					{203, 204, 122},
					{68, 135, 58},
					{80, 84, 80}};

				float bounds[numColours - 1] = {
					0.3f,
					0.33f,
					0.8f};

				sf::Color col = colours[numColours - 1];
				c->backgroundTiles[x][y].type = (BackgroundTile::BackgroundTileType)(numColours - 1);
				for (int i = 0; i < numColours - 1; i++)
				{
					if (noise < bounds[i])
					{
						c->backgroundTiles[x][y].type = (BackgroundTile::BackgroundTileType)(i);
						col = colours[i];
						break;
					}
				}

				// --- COLOR OFFSET ADDITION ---
				int offset = GetTileOffset(worldX, worldY, seed, colorVariation);

				// Apply offset and clamp RGB to valid 0-255 ranges
				col.r = static_cast<uint8_t>(std::clamp(col.r + offset, 0, 255));
				col.g = static_cast<uint8_t>(std::clamp(col.g + offset, 0, 255));
				col.b = static_cast<uint8_t>(std::clamp(col.b + offset, 0, 255));

				c->backgroundTiles[x][y].color = col;
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