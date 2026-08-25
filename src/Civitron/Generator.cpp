#include "Generator.hpp"
#include "TileInfo.hpp"
#include "../Timer.hpp"
#include "SaveManager.hpp"
#include "Utils.hpp"
namespace Civitron
{
	PartialChunk::PartialChunk(sf::Vector3i position)
	{
		phase = PHASE_NONE;
		chunk = new Chunk(position);
	}
	PartialChunk::~PartialChunk()
	{
	}
	VerticalSlice::VerticalSlice(sf::Vector2i position, Generator *generator)
	{
		treesGenerated = false;
		seed = generator->seed + position.x * position.x * position.x + (position.y << 17);
		this->position = position;
		treePoses = {};
		constexpr int minHeight = -2;
		constexpr int range = 20;
		constexpr double scale = 0.01;
		for (int x = 0; x < CHUNK_SIZE; x++)
		{
			for (int z = 0; z < CHUNK_SIZE; z++)
			{
				int height = minHeight + (int)(generator->noise.octave2D_01((x + position.x * CHUNK_SIZE) * scale, (z + position.y * CHUNK_SIZE) * scale, 8) * range);
				heights[x][z] = height;
			}
		}
	}
	void VerticalSlice::WriteChunksData(std::string &path)
	{
		for (auto &c : chunks)
		{
			std::string p = path + "/" + std::to_string(position.x) + " " + std::to_string(c.first) + " " + std::to_string(position.y) + ".txt";
			auto bytes = c.second->chunk->ToBytes();
			std::ofstream out(p);
			out.write(reinterpret_cast<const char *>(bytes.data()), bytes.size());
			out.close();
		}
	}
	nlohmann::json VerticalSlice::ToJson()
	{
		nlohmann::json j;
		j["position"] = {position.x, position.y};
		std::vector<nlohmann::json> arr = {};
		for (auto &c : chunks)
		{
			nlohmann::json j2;
			j2["height"] = c.first;
			j2["phase"] = c.second->phase;
			arr.push_back(j2);
		}
		j["partialChunks"] = arr;
		j["heights"] = heights;
		j["treePoses"] = {};
		j["seed"] = seed;
		j["treesGenerated"] = treesGenerated;
		for (int i = 0; i < treePoses.size(); i++)
		{
			j["treePoses"].push_back({treePoses[i].x, treePoses[i].y});
		}
		return j;
	}
	VerticalSlice::VerticalSlice(nlohmann::json data, std::string &chunksPath)
	{
		position = {data["position"][0], data["position"][1]};
		seed = data["seed"];
		treesGenerated = data["treesGenerated"];
		for (auto &c : data["partialChunks"])
		{
			int height = c["height"];
			GenerationPhase phase = c["phase"];
			std::string path = chunksPath + std::to_string(position.x) + " " + std::to_string(height) + " " + std::to_string(position.y) + ".txt";
			std::array<uint8_t, CHUNK_NUM_BYTES> bytes = {};
			std::ifstream in(path, std::ios::binary);
			in.read(reinterpret_cast<char *>(bytes.data()), bytes.size());
			// chunks[vec] = new Chunk(vec);
			// chunks[vec]->FromBytes(bytes);
			sf::Vector3i pos = {position.x, height, position.y};
			PartialChunk *chunk = new PartialChunk(pos);
			chunk->chunk->FromBytes(bytes);
			chunk->phase = phase;
			chunks[height] = chunk;
		}
		for (auto &p : data["treePoses"])
		{
			treePoses.push_back({p[0], p[1]});
		}
		for (int x = 0; x < CHUNK_SIZE; x++)
		{
			for (int z = 0; z < CHUNK_SIZE; z++)
			{
				heights[x][z] = data["heights"][x][z];
			}
		}
	}

	Generator::Generator()
	{
	}
	void Generator::SetSeed(uint64_t seed)
	{
		this->seed = seed;
		noise = siv::PerlinNoise(seed);
	}
	Chunk *Generator::GenerateChunk(sf::Vector3i position)
	{
		sf::Vector2i slice = {position.x, position.z};
		if (!slices.contains(slice))
		{
			slices[slice] = std::make_unique<VerticalSlice>(slice, this);
		}
		if (!slices[slice]->chunks.contains(position.y))
		{
			PartialChunk *c = new PartialChunk(position);
			slices[slice]->chunks[position.y] = c;
		}
		auto c = slices[slice]->chunks[position.y];
		LocationsPass(slices[slice].get());
		HeightPass(slices[slice].get(), c);
		DecorationPass(slices[slice].get(), c);
		Chunk *chunk = c->chunk;
		delete c;
		// slices[slice]->chunks[position.y] = std::unique_ptr<PartialChunk>();
		slices[slice]->chunks.erase(position.y);
		return chunk;
	}
	void Generator::HeightPass(VerticalSlice *slice, PartialChunk *chunk)
	{
		// VerticalSlice *slice = slices[{chunk->chunk->position.x, chunk->chunk->position.z}];
		for (int x = 0; x < CHUNK_SIZE; x++)
		{
			for (int z = 0; z < CHUNK_SIZE; z++)
			{
				int height = std::min(slice->heights[x][z] - chunk->chunk->position.y * CHUNK_SIZE, CHUNK_SIZE);
				for (int y = 0; y < height; y++)
				{
					chunk->chunk->tiles[x][y][z].type = GetTileID("Stone");
				}
			}
		}
	}

	void Generator::LocationsPass(VerticalSlice *slice)
	{
		if (!slice->treesGenerated)
		{
			slice->treesGenerated = true;
			// offset so different parts of generation don't get same numbers
			uint64_t seed = slice->seed + 1;
			int numTrees = GetNextRandom(seed) % 5;
			for (int i = 0; i < numTrees; i++)
			{
				int buffer = 2;
				int minDist = 4;
				while (true)
				{
					sf::Vector2i pos = {(int)(GetNextRandom(seed) % (CHUNK_SIZE - 2 * buffer) + buffer), (int)(GetNextRandom(seed) % (CHUNK_SIZE - 2 * buffer) + buffer)};
					bool valid = true;
					for (int i = 0; i < slice->treePoses.size(); i ++){
						if (abs(slice->treePoses[i].x - pos.x) < minDist || abs(slice->treePoses[i].y - pos.y) < minDist){
							valid = false;
							break;
						}
					}
					if (valid){
						slice->treePoses.push_back(pos);
						break;
					}
				}
			}
		}
	}

	void Generator::DecorationPass(VerticalSlice *slice, PartialChunk *chunk)
	{
		// place grass and dirt
		for (int x = 0; x < CHUNK_SIZE; x++)
		{
			for (int z = 0; z < CHUNK_SIZE; z++)
			{
				int h = slice->heights[x][z] - chunk->chunk->position.y * CHUNK_SIZE;
				for (int y = 0; y < 4; y++)
				{
					if (h - y >= CHUNK_SIZE)
					{
						continue;
					}
					if (h - y < 0)
					{
						break;
					}
					if (y == 0)
					{
#define SHOW_CHUNK_BORDERS
#ifdef SHOW_CHUNK_BORDERS
						if ((x == 0 || x == CHUNK_SIZE - 1) || (z == 0 || z == CHUNK_SIZE - 1))
						{
							chunk->chunk->tiles[x][h - y][z].type = GetTileID("TestTile");
						}
						else
						{
#endif
							chunk->chunk->tiles[x][h - y][z].type = GetTileID("Grass");
#ifdef SHOW_CHUNK_BORDERS
						}
#endif
					}
					else
					{
						chunk->chunk->tiles[x][h - y][z].type = GetTileID("Dirt");
					}
				}
			}
		}
		for (int cx = -1; cx <= 1; cx++)
		{
			for (int cz = -1; cz <= 1; cz++)
			{
				sf::Vector2i slicePos = slice->position + sf::Vector2i(cx, cz);
				if (!slices.contains(slicePos))
				{
					slices[slicePos] = std::make_unique<VerticalSlice>(slicePos, this);
					LocationsPass(slices[slicePos].get());
				}
				VerticalSlice *curr_slice = slices[slicePos].get();
				for (int i = 0; i < curr_slice->treePoses.size(); i++)
				{
					int treeHeight = 6;
					sf::Vector2i p = curr_slice->treePoses[i];
					int height = curr_slice->heights[p.x][p.y] + 1;
					sf::Vector3i pos = {p.x, height + chunk->chunk->position.y * CHUNK_SIZE, p.y};
					pos += sf::Vector3i(cx * CHUNK_SIZE, 0, cz * CHUNK_SIZE);

					// place leaves
					for (int x = -2; x <= 2; x++)
					{
						for (int z = -2; z <= 2; z++)
						{
							for (int y = -2; y <= 2; y++)
							{
								if (y >=0 && (abs(x) > 1 || abs(z) > 1)){
									continue;
								}
								sf::Vector3i p(x, treeHeight + y, z);
								p += pos;
								sf::Vector3i chunkPos = TileToChunkPos(p);
								if (chunkPos == sf::Vector3i(0, 0, 0))
								{
									chunk->chunk->tiles[p.x][p.y][p.z] = GetTileID("Leaves");
								}
								else if (cx == 0 && cz == 0)
								{
									sf::Vector3i absChunkPos = chunkPos + chunk->chunk->position;
									PartialChunk *c = GetParitalChunk(absChunkPos);
									p -= chunkPos * CHUNK_SIZE;
									c->chunk->tiles[p.x][p.y][p.z] = GetTileID("Leaves");
								}
							}
						}
					}
					// placing logs
					if (cx == 0 && cz == 0)
					{
						for (int j = 0; j < treeHeight; j++)
						{
							if (pos.y + j < 0)
							{
								continue;
							}
							if (pos.y + j >= CHUNK_SIZE)
							{
								break;
							}
							chunk->chunk->tiles[pos.x][pos.y + j][pos.z].type = GetTileID("Log");
						}
					}
				}
			}
		}
	}
	void Generator::Save(std::string &path)
	{

		nlohmann::json j;
		j["seed"] = seed;
		j["slices"] = {};
		for (auto &s : slices)
		{
			// std::string key = std::to_string(s.first.x) + " " + std::to_string(s.first.y);
			s.second->WriteChunksData(path);
			j["slices"].push_back(s.second->ToJson());
		}
		SaveManager::WriteData(path + "generator.json", j.dump(1));
	}
	void Generator::Load(std::string &path)
	{
		std::ifstream in(path + "generator.json");
		nlohmann::json j = nlohmann::json::parse(in);
		seed = j["seed"];
		for (auto &slice : j["slices"])
		{
			sf::Vector2i pos = {slice["position"][0], slice["position"][1]};
			VerticalSlice *v = new VerticalSlice(slice, path);
			slices[pos] = std::unique_ptr<VerticalSlice>(v);
		}
	}
	uint64_t Generator::GetNextRandom(uint64_t &x)
	{
		x += 67;
		x ^= x << 13;
		x ^= x >> 7;
		x ^= x << 17;
		return x;
	}

	PartialChunk *Generator::GetParitalChunk(sf::Vector3i position)
	{
		sf::Vector2i slice(position.x, position.z);
		if (!slices.contains(slice))
		{
			slices[slice] = std::make_unique<VerticalSlice>(slice, this);
		}
		if (!slices[slice]->chunks.contains(position.y))
		{
			slices[slice]->chunks[position.y] = new PartialChunk(position);
		}
		return slices[slice]->chunks[position.y];
	}

}