#pragma once
#include "../PCH.hpp"
#include "Chunk.hpp"
#include "PerlinNoise.hpp"
#include "../json.hpp"
namespace cc
{
	enum class GenerationStage : uint16_t
	{
		SET_TYPES = 0,
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
		//should be passed in as "/partial chunks/"
		void Save(std::string path);
		void Load(std::string path);
		nlohmann::json ToJson();
		void FromJson(nlohmann::json& j);
		//whether next stage is 
		bool NextStageGreaterOrEqual(GenerationStage stage);
		// std::vector<sf::Vector2i> GetConnectedTiles(sf::Vector2i startPos, BackgroundTileType type);
	};
	class Planet;
	class Generator
	{
	public:
		std::unordered_map<sf::Vector2i, std::unique_ptr<PartialChunk>,ChunkHash> partialChunks = {};
		Planet* planet;
		uint64_t seed;
		Generator();
		void SetSeed(uint64_t seed);
		nlohmann::json ToJson();
		void FromJson(nlohmann::json j);
		void Save(std::string path);
		void Load(std::string path);
		void AddPartialChunk(PartialChunk* p);
		Chunk *GenerateChunk(sf::Vector2i position);
		// BackgroundTile* GetBackgroundTileAt(sf::Vector2i position, GenerationStage minStage);
		// void SetBackgroundTileAt(sf::Vector2i position, BackgroundTile backgroundTile);
	};

}