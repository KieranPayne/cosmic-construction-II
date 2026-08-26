#pragma once
#include "../PCH.hpp"
#include "Chunk.hpp"
#include "PerlinNoise.hpp"

namespace Civitron
{

	// enum GenerationPhase : int
	// {
	// 	PHASE_NONE = -1,
	// 	PHASE_HEIGHT_MAP,
	// 	PHASE_DECORATION
	// };

	// // a chunk that's not yet completely generated
	// class PartialChunk
	// {
	// public:
	// 	Chunk *chunk;
	// 	GenerationPhase phase;
	// 	PartialChunk(sf::Vector3i position);
	// 	~PartialChunk();
	// };

	// class Generator;

	// class VerticalSlice
	// {
	// public:
	// int heights[CHUNK_SIZE][CHUNK_SIZE];
	// sf::Vector2i position;
	// 	uint64_t seed;
	// 	std::vector<sf::Vector2i> treePoses;
	// 	bool treesGenerated;
	// 	std::unordered_map<int, PartialChunk*> chunks;
	// 	VerticalSlice(sf::Vector2i position, Generator *generator);
	// 	VerticalSlice(nlohmann::json data, std::string& chunksPath);
	// 	void WriteChunksData(std::string& path);
	// 	nlohmann::json ToJson();
		
	// };

	// struct SliceHash
	// {
	// 	std::size_t operator()(const sf::Vector2i &v) const
	// 	{
	// 		return v.x + (v.y << 8);
	// 	}
	// };

	class Generator
	{
	public:
		// uint64_t seed;
		// siv::PerlinNoise noise;
		// std::unordered_map<sf::Vector2i, std::unique_ptr<VerticalSlice>, SliceHash> slices;
		Generator();
		// void SetSeed(uint64_t seed);
		Chunk *GenerateChunk(sf::Vector2i position);
		// //fills in the terrain with stone up to ground height
		// void HeightPass(VerticalSlice* slice, PartialChunk *chunk);
		// //generates the positions of the trees
		// void LocationsPass(VerticalSlice* slice);
		// //decorates the ground and places the trees
		// void DecorationPass(VerticalSlice* slice,PartialChunk *chunk);
		// void Save(std::string& path);
		// void Load(std::string& path);
		// PartialChunk* GetParitalChunk(sf::Vector3i pos);	
		// uint64_t GetNextRandom(uint64_t& x);
	};

}