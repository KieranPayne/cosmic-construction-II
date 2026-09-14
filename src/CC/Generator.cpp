#include "Generator.hpp"
#include "TileInfo.hpp"
#include "../Timer.hpp"
#include "SaveManager.hpp"
#include "Utils.hpp"
#include "PerlinNoise.hpp"
#include <queue>
namespace cc
{
	PartialChunk::PartialChunk(Generator *generator, sf::Vector2i position)
	{
		this->generator = generator;
		nextStage = GenerationStage::SET_TYPES;
		this->position = position;
	}
	void PartialChunk::GenerateToStage(GenerationStage stage)
	{
		while ((uint16_t)nextStage < (uint16_t)stage)
		{
			GenerateNextStage();
		}
	}
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
	void PartialChunk::GenerateNextStage()
	{
		constexpr std::array<double, 8> TERRAIN_BOUNDS = {
			0.0,
			0.20,
			0.35,
			0.40,
			0.70,
			0.80,
			0.90,
			1000.0};

		constexpr std::array<sf::Color, 8> TERRAIN_COLORS = {
			sf::Color{52, 140, 235},  // WATER
			sf::Color{52, 140, 235},  // WATERY SAND
			sf::Color{184, 182, 132}, // SAND
			sf::Color{61, 135, 72},	  // GRASS
			sf::Color{61, 135, 72},	  // GRASSY STONE
			sf::Color{78, 82, 79},	  // STONE
			sf::Color{38, 42, 39},	  // TALL STONE
			sf::Color{38, 42, 39}	  // END
		};

		constexpr std::array<bool, 7> TERRAIN_VARIATION = {
			false, // WATER
			false, // WATERY SAND
			true, // SAND
			true,  // GRASS
			true, // GRASSY STONE
			false, // STONE
			false  // TALL STONE
		};

		constexpr int VARIATION_AMOUNT = 15;
		constexpr double TERRAIN_NOISE_SCALE = 0.03;

		// Offset for the second noise sample.
		// It uses the same Perlin generator/seed, but samples a different
		// part of the noise field so the pattern doesn't line up with terrain.
		constexpr double GRASS_NOISE_OFFSET_X = 137.42;
		constexpr double GRASS_NOISE_OFFSET_Y = 791.83;
		constexpr double GRASS_NOISE_SCALE = 0.08;

		auto lerpColor = [](const sf::Color &a, const sf::Color &b, float t)
		{
			return sf::Color{
				static_cast<std::uint8_t>(a.r + (b.r - a.r) * t),
				static_cast<std::uint8_t>(a.g + (b.g - a.g) * t),
				static_cast<std::uint8_t>(a.b + (b.b - a.b) * t)};
		};

		if (nextStage == GenerationStage::SET_TYPES)
		{
			chunk = std::make_unique<Chunk>(position);

			const siv::PerlinNoise perlin(generator->seed);

			for (int x = 0; x < CHUNK_SIZE; ++x)
			{
				for (int y = 0; y < CHUNK_SIZE; ++y)
				{
					const int worldX = position.x * CHUNK_SIZE + x;
					const int worldY = position.y * CHUNK_SIZE + y;

					const double noise = perlin.octave2D_01(
						worldX * TERRAIN_NOISE_SCALE,
						worldY * TERRAIN_NOISE_SCALE,
						4,
						0.5);

					auto &tile = chunk->backgroundTiles[x][y];

					std::size_t terrainType = TERRAIN_BOUNDS.size() - 2;

					for (std::size_t i = 0; i < TERRAIN_BOUNDS.size() - 1; ++i)
					{
						if (noise < TERRAIN_BOUNDS[i + 1])
						{
							terrainType = i;
							break;
						}
					}

					tile.type = static_cast<BackgroundTileType>(terrainType);

					float transition =
						static_cast<float>(
							(noise - TERRAIN_BOUNDS[terrainType]) /
							(TERRAIN_BOUNDS[terrainType + 1] - TERRAIN_BOUNDS[terrainType]));

					// Quantise transition into 5 steps.
					transition = std::round(transition * 5.0f) / 5.0f;

					tile.color = sf::Color{
						static_cast<std::uint8_t>(
							std::clamp(transition, 0.0f, 1.0f) * 255.0f),
						0,
						0};
				}
			}
		}
		else if (nextStage == GenerationStage::SET_TILE_COLORS)
		{
			const siv::PerlinNoise perlin(generator->seed);

			for (int x = 0; x < CHUNK_SIZE; ++x)
			{
				for (int y = 0; y < CHUNK_SIZE; ++y)
				{
					const int worldX = position.x * CHUNK_SIZE + x;
					const int worldY = position.y * CHUNK_SIZE + y;

					auto &tile = chunk->backgroundTiles[x][y];

					const std::size_t type =
						static_cast<std::size_t>(tile.type);

					const float transition =
						tile.color.r / 255.0f;

					tile.color = lerpColor(
						TERRAIN_COLORS[type],
						TERRAIN_COLORS[type + 1],
						transition);

					if (TERRAIN_VARIATION[type])
					{
						// Same Perlin generator/seed as terrain generation,
						// but sampled from a different region of the noise field.
						const double variationNoise = perlin.octave2D_01(
							worldX * GRASS_NOISE_SCALE + GRASS_NOISE_OFFSET_X,
							worldY * GRASS_NOISE_SCALE + GRASS_NOISE_OFFSET_Y,
							3,
							0.5);

						// Convert [0, 1] -> [-VARIATION_AMOUNT, +VARIATION_AMOUNT]
						const int offset = static_cast<int>(
							std::round(
								(variationNoise * 2.0 - 1.0) *
								VARIATION_AMOUNT));

						tile.color = {
							static_cast<std::uint8_t>(
								std::clamp(
									static_cast<int>(tile.color.r) + offset,
									0,
									255)),
							static_cast<std::uint8_t>(
								std::clamp(
									static_cast<int>(tile.color.g) + offset,
									0,
									255)),
							static_cast<std::uint8_t>(
								std::clamp(
									static_cast<int>(tile.color.b) + offset,
									0,
									255))};
					}
				}
			}
		}

		nextStage = static_cast<GenerationStage>(
			static_cast<std::uint16_t>(nextStage) + 1);
	}

	void PartialChunk::Save(std::string path)
	{
		std::string coords = std::to_string(position.x) + " " + std::to_string(position.y);
		chunk->WriteData(path + coords + ".txt");
		SaveManager::WriteData(path + coords + ".json", ToJson().dump());
	}
	void PartialChunk::Load(std::string path)
	{
		chunk = std::make_unique<Chunk>(position);
		std::string coords = std::to_string(position.x) + " " + std::to_string(position.y);
		chunk->ReadData(path + coords + ".txt");
		nlohmann::json j = nlohmann::json::parse(SaveManager::ReadData(path + coords + ".json"));
		FromJson(j);
	}
	nlohmann::json PartialChunk::ToJson()
	{
		nlohmann::json j;
		j["position"] = {position.x, position.y};
		j["nextStage"] = (uint16_t)nextStage;
		return j;
	}
	void PartialChunk::FromJson(nlohmann::json &j)
	{
		position = {j["position"][0], j["position"][1]};
		nextStage = (GenerationStage)j["nextStage"];
	}
	bool PartialChunk::NextStageGreaterOrEqual(GenerationStage stage)
	{
		return ((uint16_t)nextStage >= (uint16_t)stage);
	}
	Generator::Generator()
	{
	}
	// Helper function: Deterministic 2D hash to generate a offset between [-maxOffset, maxOffset]

	Chunk *Generator::GenerateChunk(sf::Vector2i position)
	{
		PartialChunk *p;
		if (partialChunks.contains(position))
		{
			p = partialChunks[position].get();
		}
		else
		{
			p = new PartialChunk(this, position);
			AddPartialChunk(p);
		}
		p->GenerateToStage(GenerationStage::FINISHED);
		Chunk *c = p->chunk.get();
		p->chunk.release();
		partialChunks.erase(position);
		return c;
		// Chunk *c = new Chunk(position);
		// siv::PerlinNoise perlin(seed);

		// const double noiseScale = 0.03;

		// // Intensity of color variation (+/- RGB value shift)
		// const int colorVariation = 3;

		// for (int y = 0; y < CHUNK_SIZE; y++)
		// {
		// 	for (int x = 0; x < CHUNK_SIZE; x++)
		// 	{
		// 		int worldX = position.x * CHUNK_SIZE + x;
		// 		int worldY = position.y * CHUNK_SIZE + y;

		// 		double noise = perlin.octave2D_01(
		// 			worldX * noiseScale,
		// 			worldY * noiseScale,
		// 			3, 0.2);

		// 		int numColours = 4;
		// 		sf::Color colours[numColours] = {
		// 			{51, 128, 176},
		// 			{203, 204, 122},
		// 			{68, 135, 58},
		// 			{80, 84, 80}};

		// 		float bounds[numColours - 1] = {
		// 			0.35f,
		// 			0.4f,
		// 			0.8f};

		// 		sf::Color col = colours[numColours - 1];
		// 		c->backgroundTiles[x][y].type = (BackgroundTileType)(numColours - 1);
		// 		for (int i = 0; i < numColours - 1; i++)
		// 		{
		// 			if (noise < bounds[i])
		// 			{
		// 				c->backgroundTiles[x][y].type = (BackgroundTileType)(i);
		// 				col = colours[i];
		// 				break;
		// 			}
		// 		}

		// 		// --- COLOR OFFSET ADDITION ---
		// 		int offset = GetTileOffset(worldX, worldY, seed, colorVariation);

		// 		// Apply offset and clamp RGB to valid 0-255 ranges
		// 		col.r = static_cast<uint8_t>(std::clamp(col.r + offset, 0, 255));
		// 		col.g = static_cast<uint8_t>(std::clamp(col.g + offset, 0, 255));
		// 		col.b = static_cast<uint8_t>(std::clamp(col.b + offset, 0, 255));

		// 		c->backgroundTiles[x][y].color = col;
		// 	}
		// }

		// return c;
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
		if (!SaveManager::DirExists(path + "/partial chunks"))
		{
			SaveManager::CreateDirectory(path + "/partial chunks");
		}
		for (auto &p : partialChunks)
		{
			p.second->Save(path + "/partial chunks/");
		}
	}
	void Generator::Load(std::string path)
	{
		std::string data = SaveManager::ReadData(path + "/generator.json");
		nlohmann::json j = nlohmann::json::parse(data);
		FromJson(j);
		std::string chunkPath = path + "/partial chunks";
		auto files = SaveManager::ListFiles(chunkPath);
		for (auto &f : files)
		{
			// lazy way to check if its a json
			if (f.back() == 'n')
			{
				continue;
			}
			auto coords = Split(f.substr(chunkPath.size() + 1), ' ');
			// remove file extension
			coords[1] = coords[1].substr(0, coords[1].size() - 4);
			sf::Vector2i vec{std::stoi(coords[0]), std::stoi(coords[1])};
			PartialChunk *p = new PartialChunk(this, vec);
			// p->position = vec;
			AddPartialChunk(p);
			p->Load(chunkPath + "/");
		}
	}
	// std::vector<sf::Vector2i> PartialChunk::GetConnectedTiles(sf::Vector2i startPos, BackgroundTileType type)
	// {
	// 	std::queue<sf::Vector2i> toVisit;
	// 	std::unordered_set<sf::Vector2i, ChunkHash> added;
	// 	std::vector<sf::Vector2i> result;

	// 	toVisit.push(startPos);
	// 	added.emplace(startPos);
	// 	static sf::Vector2i offsets[4] =
	// 		{
	// 			{1, 0},
	// 			{-1, 0},
	// 			{0, 1},
	// 			{0, -1}};
	// 	while (!toVisit.empty())
	// 	{
	// 		sf::Vector2i currPos = toVisit.front();
	// 		result.push_back(currPos);
	// 		// visited.emplace(currPos);
	// 		toVisit.pop();
	// 		for (int i = 0; i < 4; i++)
	// 		{
	// 			sf::Vector2i newPos = currPos + offsets[i];
	// 			sf::Vector2i chunkPos = TileToChunkPos(newPos);
	// 			sf::Vector2i subChunkPos = newPos - chunkPos * CHUNK_SIZE;
	// 			if (added.contains(newPos))
	// 			{
	// 				continue;
	// 			}
	// 			bool isCorrectType = false;
	// 			if (chunkPos == position)
	// 			{
	// 				if (chunk->backgroundTiles[subChunkPos.x][subChunkPos.y].type == type)
	// 				{
	// 					isCorrectType = true;
	// 				}
	// 			}
	// 			else
	// 			{
	// 				if (generator->GetBackgroundTileAt(newPos, GenerationStage::REMOVE_SMALL_AREAS)->type == type)
	// 				{
	// 					isCorrectType = true;
	// 				}
	// 			}
	// 			if (isCorrectType)
	// 			{
	// 				toVisit.push(newPos);
	// 				added.emplace(newPos);
	// 			}
	// 		}
	// 	}
	// 	return result;
	// }
	void Generator::AddPartialChunk(PartialChunk *p)
	{
		partialChunks[p->position] = std::unique_ptr<PartialChunk>(p);
	}
	// BackgroundTile *Generator::GetBackgroundTileAt(sf::Vector2i position, GenerationStage minStage)
	// {
	// 	sf::Vector2i chunkPos = TileToChunkPos(position);
	// 	sf::Vector2i subChunkPos = position - chunkPos * CHUNK_SIZE;
	// 	if (planet->chunks.contains(chunkPos))
	// 	{
	// 		return &planet->chunks[chunkPos]->backgroundTiles[subChunkPos.x][subChunkPos.y];
	// 	}
	// 	else
	// 	{
	// 		PartialChunk *p;
	// 		if (partialChunks.contains(chunkPos))
	// 		{
	// 			p = partialChunks[chunkPos].get();
	// 		}
	// 		else
	// 		{
	// 			p = new PartialChunk(this, chunkPos);
	// 			AddPartialChunk(p);
	// 		}
	// 		if (!p->NextStageGreaterOrEqual(GenerationStage::REMOVE_SMALL_AREAS))
	// 		{
	// 			p->GenerateToStage(GenerationStage::REMOVE_SMALL_AREAS);
	// 		}
	// 		return &p->chunk->backgroundTiles[subChunkPos.x][subChunkPos.y];
	// 	}
	// }
	// void Generator::SetBackgroundTileAt(sf::Vector2i position, BackgroundTile backgroundTile)
	// {
	// 	sf::Vector2i chunkPos = TileToChunkPos(position);
	// 	sf::Vector2i subChunkPos = position - chunkPos * CHUNK_SIZE;
	// 	if (planet->chunks.contains(chunkPos))
	// 	{
	// 		planet->chunks[chunkPos]->backgroundTiles[subChunkPos.x][subChunkPos.y] = backgroundTile;
	// 	}
	// 	else
	// 	{
	// 		PartialChunk *p;
	// 		if (partialChunks.contains(chunkPos))
	// 		{
	// 			p = partialChunks[chunkPos].get();
	// 		}
	// 		else
	// 		{
	// 			p = new PartialChunk(this, chunkPos);
	// 			AddPartialChunk(p);
	// 		}
	// 		if (!p->NextStageGreaterOrEqual(GenerationStage::REMOVE_SMALL_AREAS))
	// 		{
	// 			p->GenerateToStage(GenerationStage::REMOVE_SMALL_AREAS);
	// 		}
	// 		p->chunk->backgroundTiles[subChunkPos.x][subChunkPos.y] = backgroundTile;
	// 	}
	// }
}
