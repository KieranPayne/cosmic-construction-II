#include "Generator.hpp"
#include "TileInfo.hpp"
#include "../Timer.hpp"
#include "SaveManager.hpp"
#include "Utils.hpp"
#include "PerlinNoise.hpp"
namespace cc
{
	PartialChunk::PartialChunk(Generator *generator, sf::Vector2i position)
	{
		this->generator = generator;
		nextStage = GenerationStage::WATER_AND_STONE;
		this->position = position;
	}
	void PartialChunk::GenerateNextStage()
	{
		if (nextStage == GenerationStage::WATER_AND_STONE)
		{
			chunk = std::make_unique<Chunk>(position);
			siv::PerlinNoise perlin(generator->seed);

			const double noiseScale = 0.03;
			for (int x = 0; x < CHUNK_SIZE; x++)
			{
				for (int y = 0; y < CHUNK_SIZE; y++)
				{
					int worldX = position.x * CHUNK_SIZE + x;
					int worldY = position.y * CHUNK_SIZE + y;

					double noise = perlin.octave2D_01(
						worldX * noiseScale,
						worldY * noiseScale,
						3, 0.2);
					float bounds[2] = {
						0.35f,
						0.8f};
					chunk->backgroundTiles[x][y].type = BackgroundTileType::WATER;
					for (int i = 0; i < 2; i++)
					{
						if (noise < bounds[i])
						{
							chunk->backgroundTiles[x][y].type = (BackgroundTileType)(i);
							// col = colours[i];
							break;
						}
					}
				}
			}
		}else if (nextStage == GenerationStage::REMOVE_SMALL_AREAS)
		{
			for (int x = 0; x < CHUNK_SIZE; x ++)
			{
				for (int y = 0; y < CHUNK_SIZE; y ++)
				{
					if (chunk->backgroundTiles[x][y].type == BackgroundTileType::WATER)
					{

					}
				}
			}
		}
	}

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
					3, 0.2);

				int numColours = 4;
				sf::Color colours[numColours] = {
					{51, 128, 176},
					{203, 204, 122},
					{68, 135, 58},
					{80, 84, 80}};

				float bounds[numColours - 1] = {
					0.35f,
					0.4f,
					0.8f};

				sf::Color col = colours[numColours - 1];
				c->backgroundTiles[x][y].type = (BackgroundTileType)(numColours - 1);
				for (int i = 0; i < numColours - 1; i++)
				{
					if (noise < bounds[i])
					{
						c->backgroundTiles[x][y].type = (BackgroundTileType)(i);
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
	void Generator::Save(std::string path)
	{
		nlohmann::json j = ToJson();
		SaveManager::WriteData(path + "/generator.json", j.dump());
		// TODO: SAVE PARTIAL CHUNKS
	}
	void Generator::Load(std::string path)
	{
		std::string data = SaveManager::ReadData(path + "/generator.json");
		nlohmann::json j = nlohmann::json::parse(data);
		FromJson(j);
		// TODO: LOAD PARTIAL CHUNKS
	}
}