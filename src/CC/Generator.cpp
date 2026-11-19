#include "Generator.hpp"
#include "TileInfo.hpp"
#include "../Timer.hpp"
#include "SaveManager.hpp"
#include "Utils.hpp"
#include "PerlinNoise.hpp"
namespace cc
{
	namespace
	{
		// Terrain type i covers noise values from TERRAIN_BOUNDS[i] up to TERRAIN_BOUNDS[i + 1].
		constexpr std::array<double, 8> TERRAIN_BOUNDS = {
			0.0,
			0.20,
			0.35,
			0.40,
			0.70,
			0.80,
			0.90,
			1000.0};

		// Terrain type i blends from TERRAIN_COLORS[i] at its lower bound to TERRAIN_COLORS[i + 1] at its upper bound.
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

		// Whether tiles of each terrain type get a random brightness variation.
		constexpr std::array<bool, 7> TERRAIN_VARIATION = {
			false, // WATER
			false, // WATERY SAND
			true,  // SAND
			true,  // GRASS
			true,  // GRASSY STONE
			false, // STONE
			false  // TALL STONE
		};

		// Largest brightness change (in each of red, green and blue) that variation can apply.
		constexpr int VARIATION_AMOUNT = 15;

		// How stretched the terrain noise is: smaller values give larger features.
		constexpr double TERRAIN_NOISE_SCALE = 0.01;

		// The brightness variation uses the same Perlin generator and seed as the terrain, but samples a
		// different part of the noise field (this offset) so the pattern doesn't line up with the terrain.
		constexpr double VARIATION_NOISE_OFFSET_X = 137.42;
		constexpr double VARIATION_NOISE_OFFSET_Y = 791.83;
		constexpr double VARIATION_NOISE_SCALE = 0.08;

		sf::Color LerpColor(const sf::Color &a, const sf::Color &b, float t)
		{
			return sf::Color{
				static_cast<std::uint8_t>(a.r + (b.r - a.r) * t),
				static_cast<std::uint8_t>(a.g + (b.g - a.g) * t),
				static_cast<std::uint8_t>(a.b + (b.b - a.b) * t)};
		}

		// Stage SET_TYPES: creates the chunk, picks each tile's terrain type from noise, and stores how far
		// through that type's noise range the tile is (in 5 steps) in the red channel for the next stage.
		void SetTypes(PartialChunk &p)
		{
			p.chunk = std::make_unique<Chunk>(p.position);

			const siv::PerlinNoise perlin(p.generator->seed);

			for (int x = 0; x < CHUNK_SIZE; ++x)
			{
				for (int y = 0; y < CHUNK_SIZE; ++y)
				{
					const int worldX = p.position.x * CHUNK_SIZE + x;
					const int worldY = p.position.y * CHUNK_SIZE + y;

					const double noise = perlin.octave2D_01(
						worldX * TERRAIN_NOISE_SCALE,
						worldY * TERRAIN_NOISE_SCALE,
						4,
						0.5);

					auto &tile = p.chunk->backgroundTiles[x][y];

					// find which terrain type this noise value falls in
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

					// how far through the terrain type's range the noise is, from 0 to 1
					float transition =
						static_cast<float>(
							(noise - TERRAIN_BOUNDS[terrainType]) /
							(TERRAIN_BOUNDS[terrainType + 1] - TERRAIN_BOUNDS[terrainType]));

					// quantise into 5 steps
					transition = std::round(transition * 5.0f) / 5.0f;

					tile.color = sf::Color{
						static_cast<std::uint8_t>(
							std::clamp(transition, 0.0f, 1.0f) * 255.0f),
						0,
						0};
				}
			}
		}

		// Stage SET_TILE_COLORS: turns each tile's terrain type and stored blend amount into its final
		// colour, then applies brightness variation to the terrain types that use it.
		void SetTileColors(PartialChunk &p)
		{
			const siv::PerlinNoise perlin(p.generator->seed);

			for (int x = 0; x < CHUNK_SIZE; ++x)
			{
				for (int y = 0; y < CHUNK_SIZE; ++y)
				{
					const int worldX = p.position.x * CHUNK_SIZE + x;
					const int worldY = p.position.y * CHUNK_SIZE + y;

					auto &tile = p.chunk->backgroundTiles[x][y];

					const std::size_t type = static_cast<std::size_t>(tile.type);

					// the blend amount stored in the red channel by the previous stage
					const float transition = tile.color.r / 255.0f;

					tile.color = LerpColor(
						TERRAIN_COLORS[type],
						TERRAIN_COLORS[type + 1],
						transition);

					if (TERRAIN_VARIATION[type])
					{
						const double variationNoise = perlin.octave2D_01(
							worldX * VARIATION_NOISE_SCALE + VARIATION_NOISE_OFFSET_X,
							worldY * VARIATION_NOISE_SCALE + VARIATION_NOISE_OFFSET_Y,
							3,
							0.5);

						// convert [0, 1] to [-VARIATION_AMOUNT, +VARIATION_AMOUNT]
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

		// Scrambles a 64-bit number so that similar inputs give unrelated outputs.
		static uint64_t SplitMix64(uint64_t x)
		{
			x += 0x9E3779B97F4A7C15ULL;
			x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
			x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
			return x ^ (x >> 31);
		}

		// One seed per (world seed, chunk, purpose). Different chunks, or different salts, give unrelated seeds.
		uint64_t ChunkSeed(uint64_t worldSeed, sf::Vector2i chunkPos, uint64_t salt = 0)
		{
			uint64_t h = SplitMix64(worldSeed);
			h = SplitMix64(h ^ (uint64_t)(uint32_t)chunkPos.x);
			h = SplitMix64(h ^ (uint64_t)(uint32_t)chunkPos.y); // chained, so (1,2) and (2,1) differ
			h = SplitMix64(h ^ salt);
			return h;
		}

		// `count` distinct coordinates in [0, CHUNK_SIZE-1], decided entirely by the seed and the chunk.
		std::vector<sf::Vector2i> ChunkRandomCoords(uint64_t worldSeed, sf::Vector2i chunkPos,
													int count, uint64_t salt = 0)
		{
			std::mt19937_64 rng(ChunkSeed(worldSeed, chunkPos, salt));

			int total = CHUNK_SIZE * CHUNK_SIZE;
			count = std::min(count, total);

			std::vector<int> cells(total);
			std::iota(cells.begin(), cells.end(), 0);

			std::vector<sf::Vector2i> coords;
			for (int i = 0; i < count; ++i)
			{
				// Partial shuffle: swap a random remaining cell into position i.
				int j = i + (int)(rng() % (uint64_t)(total - i));
				std::swap(cells[i], cells[j]);
				coords.push_back({cells[i] % CHUNK_SIZE, cells[i] / CHUNK_SIZE});
			}
			return coords;
		}
		double ChunkRandom01(uint64_t worldSeed, sf::Vector2i chunkPos, uint64_t salt = 0)
		{
			std::mt19937_64 rng(ChunkSeed(worldSeed, chunkPos, salt));
			return std::generate_canonical<double, 53>(rng);
		}
		void AddTallGrass(PartialChunk &p)
		{
			static uint16_t tallGrassId = TileInfo::nameLookup["Tall grass"]->id;
			static std::array<sf::Vector2i, 9> offsets({{0, 0},
														{-1, -1},
														{-1, 0},
														{-1, 1},
														{0, -1},
														{0, 1},
														{1, -1},
														{1, 0},
														{1, -1}});
			// not the actual number of trees as most will land on sand, water etc.
			int numGrassMax = 50;
			std::vector<sf::Vector2i> grassPoses = ChunkRandomCoords(p.generator->seed, p.position, numGrassMax, 2);
			int salt = 1;
			double chance = 0.8;
			for (auto &pos : grassPoses)
			{
				for (auto offset : offsets)
				{
					sf::Vector2i newPos = pos + offset;
					if (newPos.x < 0 || newPos.x >= CHUNK_SIZE || newPos.y < 0 || newPos.y >= CHUNK_SIZE)
					{
						continue;
					}
					if (p.chunk->backgroundTiles[newPos.x][newPos.y].type == BackgroundTileType::GRASS)
					{
						salt++;
						if (ChunkRandom01(p.generator->seed, p.position, salt) < chance)
						{
							p.chunk->tiles[newPos.x][newPos.y] = Tile(tallGrassId);
						}
					}
				}
			}
		}
		void AddTrees(PartialChunk &p)
		{
			static uint16_t treeId = TileInfo::nameLookup["Tree"]->id;
			// not the actual number of trees as most will land on sand, water etc.
			int numTreesMax = 20;
			std::vector<sf::Vector2i> treePoses = ChunkRandomCoords(p.generator->seed, p.position, numTreesMax);
			for (auto &pos : treePoses)
			{
				if (p.chunk->backgroundTiles[pos.x][pos.y].type == BackgroundTileType::GRASS)
				{
					p.chunk->tiles[pos.x][pos.y] = Tile(treeId);
				}
			}
		}
		void AddRocks(PartialChunk &p)
		{
			static uint16_t rockId = TileInfo::nameLookup["Rock"]->id;
			// not the actual number of trees as most will land on sand, water etc.
			int numRocksMax = 50;
			std::vector<sf::Vector2i> treePoses = ChunkRandomCoords(p.generator->seed, p.position, numRocksMax, 1);
			for (auto &pos : treePoses)
			{
				BackgroundTileType type = p.chunk->backgroundTiles[pos.x][pos.y].type;
				if (type == BackgroundTileType::STONE || type == BackgroundTileType::TALL_STONE)
				{
					p.chunk->tiles[pos.x][pos.y] = Tile(rockId);
				}
			}
		}
	}

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

	// Deterministic 2D hash that gives an offset between -maxOffset and +maxOffset.
	int GetTileOffset(int x, int y, uint32_t seed, int maxOffset = 15)
	{
		// combine coordinates and world seed into a single hash value
		uint32_t hash = static_cast<uint32_t>(x) * 73856093u ^ static_cast<uint32_t>(y) * 19349663u ^ seed * 83492791u;

		// scramble bits (Murmur3-style finalizer)
		hash ^= hash >> 16;
		hash *= 0x85ebca6b;
		hash ^= hash >> 13;
		hash *= 0xc2b2ae35;
		hash ^= hash >> 16;

		// map to the range [-maxOffset, maxOffset]
		int range = (maxOffset * 2) + 1;
		return (static_cast<int>(hash % range)) - maxOffset;
	}

	void PartialChunk::GenerateNextStage()
	{
		if (nextStage == GenerationStage::SET_TYPES)
		{
			SetTypes(*this);
		}
		else if (nextStage == GenerationStage::SET_TILE_COLORS)
		{
			SetTileColors(*this);
		}
		else if (nextStage == GenerationStage::ADD_DECORATION)
		{
			AddTallGrass(*this);
			AddTrees(*this);
			AddRocks(*this);
		}

		nextStage = static_cast<GenerationStage>(
			static_cast<std::uint16_t>(nextStage) + 1);
	}

	void PartialChunk::Save(std::string path)
	{
		std::string coords = std::to_string(position.x) + " " + std::to_string(position.y);
		chunk->WriteData(path + coords + ".txt");
		Serializer s(Serializer::Mode::WRITE, SaveManager::saveFormat);
		Serialize(s);
		SaveManager::WriteSerializerToFile(s, path + coords);
	}

	void PartialChunk::Load(std::string path)
	{
		chunk = std::make_unique<Chunk>(position);
		std::string coords = std::to_string(position.x) + " " + std::to_string(position.y);
		chunk->ReadData(path + coords + ".txt");
		Serializer s = SaveManager::LoadSerializerFromFile(path + coords);
		Serialize(s);
	}

	void PartialChunk::Serialize(Serializer &s)
	{
		s.field("position", position);
		uint16_t n = (uint16_t)nextStage;
		s.field("nextStage", n);
		nextStage = (GenerationStage)n;
	}

	bool PartialChunk::NextStageGreaterOrEqual(GenerationStage stage)
	{
		return ((uint16_t)nextStage >= (uint16_t)stage);
	}

	Chunk *Generator::GenerateChunk(sf::Vector2i position)
	{
		// continue an existing partial chunk, or start a new one
		PartialChunk *p;
		auto existing = partialChunks.find(position);
		if (existing != partialChunks.end())
		{
			p = existing->second.get();
		}
		else
		{
			p = new PartialChunk(this, position);
			AddPartialChunk(p);
		}

		p->GenerateToStage(GenerationStage::FINISHED);

		// hand the finished chunk to the caller and forget the partial chunk
		Chunk *c = p->chunk.release();
		partialChunks.erase(position);
		return c;
	}

	void Generator::SetSeed(uint64_t seed)
	{
		this->seed = seed;
	}

	void Generator::Serialize(Serializer &s)
	{
		s.field("seed", seed);
	}

	void Generator::Save(std::string path)
	{
		Serializer s(Serializer::Mode::WRITE, SaveManager::saveFormat);
		Serialize(s);
		SaveManager::WriteSerializerToFile(s, path + "/generator");

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
		Serializer s = SaveManager::LoadSerializerFromFile(path + "/generator");
		Serialize(s);

		std::string chunkPath = path + "/partial chunks";
		auto files = SaveManager::ListFiles(chunkPath);
		for (auto &f : files)
		{
			// skip the serializer data files (their extensions end in 'n', such as .json):
			// each partial chunk is found through its .txt chunk file
			if (f.back() == 'n')
			{
				continue;
			}
			auto coords = Split(f.substr(chunkPath.size() + 1), ' ');
			// remove file extension
			coords[1] = coords[1].substr(0, coords[1].size() - 4);
			sf::Vector2i vec{std::stoi(coords[0]), std::stoi(coords[1])};
			PartialChunk *p = new PartialChunk(this, vec);
			AddPartialChunk(p);
			p->Load(chunkPath + "/");
		}
	}

	void Generator::AddPartialChunk(PartialChunk *p)
	{
		partialChunks[p->position] = std::unique_ptr<PartialChunk>(p);
	}

	void Generator::AppendToPacket(sf::Packet &p)
	{
		Serializer s(Serializer::Mode::WRITE, Serializer::Format::BINARY);
		Serialize(s);
		auto bytes = s.binary();
		p << (uint64_t)bytes.size();
		p.append(bytes.data(), bytes.size());
		p << (uint64_t)partialChunks.size();
		for (auto &partial : partialChunks)
		{
			p << partial.second->position.x << partial.second->position.y;
			auto chunkData = partial.second->chunk->GetByteData();
			p << (uint64_t)chunkData.size();
			p.append(chunkData.data(), chunkData.size());
			Serializer s2(Serializer::Mode::WRITE, Serializer::Format::BINARY);
			partial.second->Serialize(s2);
			auto partialBytes = s2.binary();
			p << (uint64_t)partialBytes.size();
			p.append(partialBytes.data(), partialBytes.size());
		}
	}
	void Generator::LoadFromPacket(sf::Packet &p)
	{
		auto bytes = ReadBytesFromPacket(p);
		Serializer s(Serializer::Mode::READ, Serializer::Format::BINARY, {}, bytes);
		Serialize(s);
		uint64_t numPartials;
		p >> numPartials;
		for (int i = 0; i < numPartials; i++)
		{
			sf::Vector2i pos;
			p >> pos.x >> pos.y;
			PartialChunk *partial = new PartialChunk(this, pos);
			auto chunkData = ReadBytesFromPacket(p);
			partial->chunk->LoadByteData(chunkData);
			auto partialBytes = ReadBytesFromPacket(p);
			Serializer s2(Serializer::Mode::READ, Serializer::Format::BINARY, {}, partialBytes);
			partial->Serialize(s2);
			partialChunks[pos] = std::unique_ptr<PartialChunk>(partial);
		}
	}
	uint64_t Generator::GenerateChecksum()
	{
		Serializer s(Serializer::Mode::WRITE, Serializer::Format::BINARY);
		uint64_t seedCopy = seed;
		s.field("seed", seedCopy);

		std::vector<sf::Vector2i> positions;
		positions.reserve(partialChunks.size());
		for (const auto &[pos, _] : partialChunks)
		{
			positions.push_back(pos);
		}

		std::sort(positions.begin(), positions.end(),
				  [](const sf::Vector2i &a, const sf::Vector2i &b)
				  {
					  if (a.y != b.y)
						  return a.y < b.y;
					  return a.x < b.x;
				  });

		uint64_t partialCount = static_cast<uint64_t>(positions.size());
		s.field("partialCount", partialCount);

		for (const auto &pos : positions)
		{
			sf::Vector2i posCopy = pos;
			s.field("pos", posCopy);

			const PartialChunk *pc = partialChunks.at(pos).get();
			uint16_t stage = static_cast<uint16_t>(pc->nextStage);
			s.field("stage", stage);

			bool hasChunk = (pc->chunk != nullptr);
			s.field("hasChunk", hasChunk);
			if (hasChunk)
			{
				uint64_t chunkHash = pc->chunk->GenerateChecksum();
				s.field("chunkHash", chunkHash);
			}
		}

		auto binary = s.binary();
		return ComputeChecksum(binary.data(), binary.size());
	}
}