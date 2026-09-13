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
	void PartialChunk::GenerateNextStage()
	{
		if (nextStage == GenerationStage::SET_TYPES)
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
						4, 0.5);
					float bounds[5] = {
						0.f,
						0.35f,
						0.4f,
						0.9f,
						5.f};
					chunk->backgroundTiles[x][y].type = BackgroundTileType::WATER;
					int bound = 4;
					for (int i = 0; i < 4; i++)
					{
						if (noise < bounds[i + 1])
						{
							bound = i;
							chunk->backgroundTiles[x][y].type = (BackgroundTileType)(i);
							// col = colours[i];
							break;
						}
					}
					float transition = (noise - bounds[bound]) / (bounds[bound + 1] - bounds[bound]);
					chunk->backgroundTiles[x][y].color = {(uint8_t)(std::clamp(transition, 0.f, 1.f) * 255), 0, 0};
				}
			}
		}
		// else if (nextStage == GenerationStage::REMOVE_SMALL_AREAS)
		// {
		// 	bool clear[CHUNK_SIZE][CHUNK_SIZE] = {};
		// 	for (int x = 0; x < CHUNK_SIZE; x++)
		// 	{
		// 		for (int y = 0; y < CHUNK_SIZE; y++)
		// 		{
		// 			if (clear[x][y])
		// 			{
		// 				continue;
		// 			}
		// 			if (chunk->backgroundTiles[x][y].type == BackgroundTileType::WATER)
		// 			{
		// 				sf::Vector2i pos = position * CHUNK_SIZE + sf::Vector2i{x, y};
		// 				auto connected = GetConnectedTiles(pos, BackgroundTileType::WATER);
		// 				int count = connected.size();
		// 				static const int threshold = 10;
		// 				if (count < threshold)
		// 				{
		// 					for (auto &pos : connected)
		// 					{
		// 						generator->SetBackgroundTileAt(pos, {sf::Color::White, BackgroundTileType::GRASS});
		// 					}
		// 				}
		// 				sf::Vector2i chunkPos = TileToChunkPos(position);
		// 				for (auto& p : connected)
		// 				{
		// 					if (chunkPos == TileToChunkPos(p))
		// 					{
		// 						clear[p.x - position.x * CHUNK_SIZE][p.y - position.y * CHUNK_SIZE] = true;
		// 					}
		// 				}
		// 			}
		// 			if (chunk->backgroundTiles[x][y].type == BackgroundTileType::STONE)
		// 			{
		// 				sf::Vector2i pos = position * CHUNK_SIZE + sf::Vector2i{x, y};
		// 				auto connected = GetConnectedTiles(pos, BackgroundTileType::STONE);
		// 				int count = connected.size();
		// 				static const int threshold = 10;
		// 				if (count < threshold)
		// 				{
		// 					for (auto &pos : connected)
		// 					{
		// 						generator->SetBackgroundTileAt(pos, {sf::Color::White, BackgroundTileType::GRASS});
		// 					}
		// 				}
		// 				sf::Vector2i chunkPos = TileToChunkPos(position);
		// 				for (auto& p : connected)
		// 				{
		// 					if (chunkPos == TileToChunkPos(p))
		// 					{
		// 						clear[p.x - position.x * CHUNK_SIZE][p.y - position.y * CHUNK_SIZE] = true;
		// 					}
		// 				}
		// 			}
		// 		}
		// 	}
		// }
		// else if (nextStage == GenerationStage::SAND)
		// {
		// }
		else if (nextStage == GenerationStage::SET_TILE_COLORS)
		{
			const std::array<sf::Color, 4> colors = {
				sf::Color{52, 140, 235},
				sf::Color{184, 182, 132},
				sf::Color{61, 135, 72},
				sf::Color{78, 82, 79}};

			auto lerpColor = [](const sf::Color &a, const sf::Color &b, float t)
			{
				return sf::Color{
					static_cast<std::uint8_t>(a.r + (b.r - a.r) * t),
					static_cast<std::uint8_t>(a.g + (b.g - a.g) * t),
					static_cast<std::uint8_t>(a.b + (b.b - a.b) * t)};
			};

			for (int x = 0; x < CHUNK_SIZE; ++x)
			{
				for (int y = 0; y < CHUNK_SIZE; ++y)
				{
					auto &tile = chunk->backgroundTiles[x][y];

					const std::size_t type = static_cast<std::uint8_t>(tile.type);
					const float transition = tile.color.r / 255.0f;

					if (type == 3)
					{
						tile.color = colors[type];
					}
					else
					{
						tile.color = lerpColor(
							colors[type],
							colors[type + 1],
							transition);
					}
				}
			}
		}
		nextStage = (GenerationStage)((uint16_t)(nextStage) + 1);
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
