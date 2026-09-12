#pragma once
#include "../PCH.hpp"
#include "Chunk.hpp"
#include "PerlinNoise.hpp"
#include "../json.hpp"
namespace cc
{
	enum class GenerationStage : uint16_t
	{
		WATER_AND_STONE,
		REMOVE_SMALL_AREAS,
		SAND,
		SET_TILE_COLORS,
		FINISHED
	};
	class Generator;
	class PartialChunk
	{
		public:
		Generator* generator;
		sf::Vector2i position;
		std::unique_ptr<Chunk> chunk = std::unique_ptr<Chunk>(nullptr);
		GenerationStage nextStage;
		PartialChunk(Generator* generator, sf::Vector2i position);
		void GenerateNextStage();
		//the stage passed in is what the next stage should be when done.
		void GenerateToStage(GenerationStage stage);
		void Save(std::string path);
		void Load(std::string path);
		bool hasCompletedStage(GenerationStage stage);
	};
	class Generator
	{
	public:
		std::unordered_map<sf::Vector2i, std::unique_ptr<PartialChunk>,ChunkHash> partialChunks;
		uint64_t seed;
		Generator();
		void SetSeed(uint64_t seed);
		nlohmann::json ToJson();
		void FromJson(nlohmann::json j);
		void Save(std::string path);
		void Load(std::string path);
		Chunk *GenerateChunk(sf::Vector2i position);
	};

}